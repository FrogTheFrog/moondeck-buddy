// header file include
#include "os/steamhandler.h"

// system/Qt includes
#include <QProcess>

// local includes
#include "os/shared/nativeprocesshandlerinterface.h"
#include "os/steam/steamappwatcher.h"
#include "os/steam/steamlauncher.h"
#include "shared/loggingcategories.h"
#include "utils/appsettings.h"

namespace os
{
SteamHandler::SteamHandler(const utils::AppSettings&                      app_settings,
                           std::unique_ptr<NativeProcessHandlerInterface> process_handler_interface)
    : m_app_settings{app_settings}
    , m_steam_process_tracker{std::move(process_handler_interface)}
{
    connect(&m_steam_process_tracker, &SteamProcessTracker::signalProcessStateChanged, this,
            &SteamHandler::slotSteamProcessStateChanged);
}

SteamHandler::~SteamHandler() = default;

bool SteamHandler::isSteamReady() const
{
    return SteamLauncher::isSteamReady(m_steam_process_tracker, m_app_settings.getForceBigPicture());
}

bool SteamHandler::close()
{
    m_steam_process_tracker.slotCheckState();
    if (!m_steam_process_tracker.isRunning())
    {
        return true;
    }

    // Clear the session data as we are no longer interested in it
    clearSessionData();

    // Try to shut down steam gracefully first
    const auto& exec_path{m_app_settings.getSteamExecutablePath()};
    if (!exec_path.isEmpty())
    {
        if (QProcess::startDetached(exec_path, {"-shutdown"}))
        {
            return true;
        }

        qCWarning(lc::os) << "Failed to start Steam shutdown sequence! Using others means to close steam...";
    }
    else
    {
        qCWarning(lc::os) << "Steam EXEC path is not available yet, using other means of closing!";
    }

    m_steam_process_tracker.close();
    return true;
}

std::optional<std::tuple<uint, enums::AppState>> SteamHandler::getAppData() const
{
    if (const auto* watcher{m_session_data.m_steam_app_watcher.get()})
    {
        return std::make_tuple(watcher->getAppId(), watcher->getAppState());
    }

    return std::nullopt;
}

bool SteamHandler::launchApp(const uint app_id)
{
    const auto& exec_path{m_app_settings.getSteamExecutablePath()};
    if (exec_path.isEmpty())
    {
        qCWarning(lc::os) << "Steam EXEC path is not available yet!";
        return false;
    }

    if (app_id == 0)
    {
        qCWarning(lc::os) << "Will not launch app with 0 ID!";
        return false;
    }

    if (!m_session_data.m_steam_launcher)
    {
        m_steam_process_tracker.slotCheckState();
        m_session_data = {.m_steam_launcher{std::make_unique<SteamLauncher>(m_steam_process_tracker, exec_path,
                                                                            m_app_settings.getForceBigPicture())},
                          .m_steam_app_watcher{}};

        connect(m_session_data.m_steam_launcher.get(), &SteamLauncher::signalFinished, this,
                &SteamHandler::slotSteamLaunchFinished);
    }

    m_session_data.m_steam_launcher->setAppId(app_id);
    return true;
}

void SteamHandler::clearSessionData()
{
    m_session_data = {};
}

void SteamHandler::slotSteamProcessStateChanged()
{
    if (m_steam_process_tracker.isRunning())
    {
        qCInfo(lc::os) << "Steam is running! PID:" << m_steam_process_tracker.getPid()
                       << "| START_TIME:" << m_steam_process_tracker.getStartTime();
    }
    else
    {
        qCInfo(lc::os) << "Steam is no longer running!";
        m_session_data = {};
        emit signalSteamClosed();
    }
}

void SteamHandler::slotSteamLaunchFinished(const QString& steam_exec, const uint app_id, const bool success)
{
    if (!success)
    {
        m_session_data = {};
        return;
    }

    const bool is_app_running{SteamAppWatcher::getAppState(m_steam_process_tracker, app_id)
                              != enums::AppState::Stopped};
    if (!is_app_running
        && !SteamLauncher::executeDetached(steam_exec, QStringList{"-applaunch", QString::number(app_id)}))
    {
        qCWarning(lc::os) << "Failed to perform app launch for AppID: " << app_id;
        return;
    }

    m_session_data = {.m_steam_launcher{},
                      .m_steam_app_watcher{std::make_unique<SteamAppWatcher>(m_steam_process_tracker, app_id)}};
}
}  // namespace os
