// header file include
#include "os/steamhandler.h"

// system/Qt includes
#include <QProcess>

// local includes
#include "os/shared/nativeprocesshandlerinterface.h"
#include "os/steam/steamappwatcher.h"
#include "shared/loggingcategories.h"
#include "utils/appsettings.h"

namespace
{
bool executeDetached(const QString& steam_exec, const QStringList& args)
{
    QProcess steam_process;
    steam_process.setStandardOutputFile(QProcess::nullDevice());
    steam_process.setStandardErrorFile(QProcess::nullDevice());
    steam_process.setProgram(steam_exec);
    steam_process.setArguments(args);

    return steam_process.startDetached();
}
}  // namespace

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

bool SteamHandler::launchSteam(const bool big_picture_mode)
{
    const auto& exec_path{m_app_settings.getSteamExecutablePath()};
    if (exec_path.isEmpty())
    {
        qCWarning(lc::os) << "Steam EXEC path is not available yet!";
        return false;
    }

    m_steam_process_tracker.slotCheckState();
    if (!m_steam_process_tracker.isRunning()
        || (big_picture_mode && getSteamUiMode() != enums::SteamUiMode::BigPicture))
    {
        if (!executeDetached(exec_path, big_picture_mode ? QStringList{"steam://open/bigpicture"} : QStringList{}))
        {
            qCWarning(lc::os) << "Failed to launch Steam!";
            return false;
        }
    }

    return true;
}

enums::SteamUiMode SteamHandler::getSteamUiMode() const
{
    if (const auto* log_trackers{m_steam_process_tracker.getLogTrackers()})
    {
        return log_trackers->m_web_helper.getSteamUiMode();
    }

    return enums::SteamUiMode::Unknown;
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

std::optional<std::tuple<std::uint64_t, enums::AppState>> SteamHandler::getAppData() const
{
    if (const auto* watcher{m_session_data.m_steam_app_watcher.get()})
    {
        return std::make_tuple(watcher->getAppId(), watcher->getAppState());
    }

    return std::nullopt;
}

bool SteamHandler::launchApp(const std::uint64_t app_id)
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

    m_steam_process_tracker.slotCheckState();
    if (getSteamUiMode() == enums::SteamUiMode::Unknown)
    {
        qCWarning(lc::os) << "Steam is not running or has not reached a stable state yet!";
        return false;
    }

    const bool is_app_running{SteamAppWatcher::getAppState(m_steam_process_tracker, app_id)
                              != enums::AppState::Stopped};
    if (!is_app_running && !executeDetached(exec_path, QStringList{"steam://rungameid/" + QString::number(app_id)}))
    {
        qCWarning(lc::os) << "Failed to perform app launch for AppID: " << app_id;
        return false;
    }

    m_session_data = {.m_steam_app_watcher{std::make_unique<SteamAppWatcher>(m_steam_process_tracker, app_id)}};
    return true;
}

void SteamHandler::clearSessionData()
{
    m_session_data = {};
}

std::optional<std::map<std::uint64_t, QString>> SteamHandler::getNonSteamAppData(const std::uint64_t user_id) const
{
    qDebug() << "SteamHandler::getNonSteamAppData" << user_id;
    return std::nullopt;
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
}  // namespace os
