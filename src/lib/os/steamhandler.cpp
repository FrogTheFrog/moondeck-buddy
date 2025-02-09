// header file include
#include "os/steamhandler.h"

// system/Qt includes
#include <QProcess>
#include <QThread>

// local includes
#include "os/shared/nativeprocesshandlerinterface.h"
#include "os/steam/steamcontentlogtracker.h"
#include "os/steam/steamwebhelperlogtracker.h"
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
    // TODO: if app is running, return true

    // if (m_log_trackers.m_web_helper)
    // {
    //     const auto current_mode{m_log_trackers.m_web_helper->getUiMode()};
    //     const auto required_mode{m_app_settings.getForceBigPicture() ? SteamWebHelperLogTracker::UiMode::BigPicture
    //                                                                  : SteamWebHelperLogTracker::UiMode::Desktop};
    //     return current_mode == required_mode;
    // }

    return false;
}

bool SteamHandler::close()
{
    if (!m_steam_process_tracker.isRunningNow())
    {
        return true;
    }

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

bool SteamHandler::launchApp(uint app_id)
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

    // if (getRunningApp() != app_id)
    // {
    //     const bool force_big_picture{m_app_settings.getForceBigPicture()};
    //     const bool is_steam_running{m_steam_process_tracker->isRunningNow()};
    //     if (force_big_picture && is_steam_running)
    //     {
    //         QProcess steam_process;
    //         steam_process.setStandardOutputFile(QProcess::nullDevice());
    //         steam_process.setStandardErrorFile(QProcess::nullDevice());
    //         steam_process.setProgram(exec_path);
    //         steam_process.setArguments({"steam://open/bigpicture"});
    //
    //         if (!steam_process.startDetached())
    //         {
    //             qCWarning(lc::os) << "Failed to open Steam in big picture mode!";
    //             return false;
    //         }
    //
    //         // Need to wait a little until Steam actually goes into bigpicture mode...
    //         const uint time_it_takes_steam_to_open_big_picture{1000};
    //         QThread::msleep(time_it_takes_steam_to_open_big_picture);
    //     }
    //
    //     QProcess steam_process;
    //     steam_process.setStandardOutputFile(QProcess::nullDevice());
    //     steam_process.setStandardErrorFile(QProcess::nullDevice());
    //     steam_process.setProgram(exec_path);
    //     steam_process.setArguments((force_big_picture && !is_steam_running ? QStringList{"-bigpicture"} :
    //     QStringList{})
    //                                + QStringList{"-applaunch", QString::number(app_id)});
    //
    //     if (!steam_process.startDetached())
    //     {
    //         qCWarning(lc::os) << "Failed to start Steam app launch sequence!";
    //         return false;
    //     }
    //
    //     // m_tracked_app = TrackedAppData{app_id, false, false};
    //     // m_registry_observer->startTrackingApp(app_id);
    // }

    return true;
}

void SteamHandler::slotSteamProcessStateChanged()
{
    if (m_steam_process_tracker.isRunning())
    {
        qCInfo(lc::os) << "Steam is running! PID:" << m_steam_process_tracker.getPid()
                       << "START_TIME:" << m_steam_process_tracker.getStartTime();
    }
    else
    {
        qCInfo(lc::os) << "Steam is no longer running!";
        emit signalSteamClosed();
    }
}
}  // namespace os
