// header file include
#include "os/steamhandler.h"

// local includes
#include "os/shared/nativeprocesshandlerinterface.h"
#include "os/steam/shortcutsvdf.h"
#include "os/steam/steamappwatcher.h"
#include "shared/loggingcategories.h"
#include "utils/appsettings.h"

namespace os
{
SteamHandler::SteamHandler(const utils::AppSettings&                      app_settings,
                           std::unique_ptr<NativeProcessHandlerInterface> process_handler_interface)
    : m_command_proxy{app_settings}
    , m_steam_process_tracker{std::move(process_handler_interface)}
{
    connect(&m_steam_process_tracker, &SteamProcessTracker::signalProcessStateChanged, this,
            &SteamHandler::slotSteamProcessStateChanged);
}

SteamHandler::~SteamHandler() = default;

bool SteamHandler::launchSteam(const bool big_picture_mode, const QMap<QString, QString>& env_overrides)
{
    if (!m_command_proxy.canExecuteCommands())
    {
        qCWarning(lc::os) << "Steam commands cannot be executed yet!";
        return false;
    }

    m_steam_process_tracker.slotCheckState();
    if (!m_steam_process_tracker.isRunning()
        || (big_picture_mode && getSteamUiMode() != enums::SteamUiMode::BigPicture))
    {
        if (!m_command_proxy.launchSteam(big_picture_mode, env_overrides))
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
    if (m_command_proxy.canExecuteCommands())
    {
        if (m_command_proxy.close())
        {
            return true;
        }

        qCWarning(lc::os) << "Failed to start Steam shutdown sequence! Using others means to close steam...";
    }
    else
    {
        qCWarning(lc::os) << "Steam commands cannot be executed yet, using other means of closing!";
    }

    m_steam_process_tracker.close();
    return true;
}

bool SteamHandler::closeBigPictureMode()
{
    if (!m_command_proxy.canExecuteCommands())
    {
        qCWarning(lc::os) << "Steam commands cannot be executed yet!";
        return false;
    }

    m_steam_process_tracker.slotCheckState();
    if (m_steam_process_tracker.isRunning() && getSteamUiMode() == enums::SteamUiMode::BigPicture)
    {
        if (!m_command_proxy.closeBigPictureMode())
        {
            qCWarning(lc::os) << "Failed to close Steam's BPM!";
            return false;
        }
    }

    return true;
}

std::optional<std::tuple<shared::AppId, enums::AppState>>
    SteamHandler::getAppData(const std::optional<shared::AppId>& app_id) const
{
    if (app_id)
    {
        const auto app_state{SteamAppWatcher::getAppState(m_steam_process_tracker, *app_id)};
        if (app_state)
        {
            return std::make_tuple(*app_id, *app_state);
        }
    }
    else if (const auto* watcher{m_session_data.m_steam_app_watcher.get()})
    {
        return std::make_tuple(watcher->getAppId(), watcher->getAppState());
    }

    return std::nullopt;
}

bool SteamHandler::launchApp(const shared::AppId& app_id, const QMap<QString, QString>& env_overrides)
{
    if (!m_command_proxy.canExecuteCommands())
    {
        qCWarning(lc::os) << "Steam commands cannot be executed yet!";
        return false;
    }

    if (app_id.getId() == 0)
    {
        qCWarning(lc::os) << "Will not launch app with 0 ID!";
        return false;
    }

    m_steam_process_tracker.slotCheckState();
    const auto* log_trackers{m_steam_process_tracker.getLogTrackers()};
    if (log_trackers == nullptr)
    {
        qCWarning(lc::os) << "Steam is not running or the log trackers have not been initialized yet!";
        return false;
    }

    if (log_trackers->m_web_helper.getSteamUiMode() == enums::SteamUiMode::Unknown)
    {
        qCWarning(lc::os) << "Steam has not reached a stable UI state yet!";
        return false;
    }

    const auto current_steam_id{log_trackers->m_connection_log.getCurrentSteamId()};
    if (!current_steam_id)
    {
        qCWarning(lc::os) << "User's SteamId is not available yet - cannot launch games until user logs in!";
        return false;
    }

    if (const auto app_data{getAppData(std::nullopt)}; app_data && std::get<shared::AppId>(*app_data) != app_id)
    {
        qCWarning(lc::os) << "Buddy is already tracking app id: " << std::get<shared::AppId>(*app_data).getId();
        return false;
    }

    const bool is_app_running{
        SteamAppWatcher::getAppState(m_steam_process_tracker, app_id).value_or(enums::AppState::Stopped)
        != enums::AppState::Stopped};
    if (!is_app_running)
    {
        if (!m_command_proxy.launchApp(app_id, env_overrides))
        {
            qCWarning(lc::os) << "Failed to perform app launch for AppID: " << app_id.getId();
            return false;
        }
    }

    m_session_data = {.m_steam_app_watcher{std::make_unique<SteamAppWatcher>(m_steam_process_tracker, app_id)}};
    return true;
}

void SteamHandler::clearSessionData()
{
    qCInfo(lc::os) << "Clearing session data...";
    m_session_data = {};
}

std::optional<std::map<shared::AppId, QString>> SteamHandler::getNonSteamAppData(const shared::SteamId& user_id) const
{
    if (const auto shortcuts{ShortcutsVdfEntry::scrapeShortcutsVdf(m_steam_process_tracker.getSteamDir(), user_id)})
    {
        std::map<shared::AppId, QString> app_ids_to_app_names;

        QString     buffer;
        QTextStream stream(&buffer);
        if (!shortcuts->empty())
        {
            stream << "Found " << shortcuts->size() << " non-Steam shortcut(-s):";
            for (const auto& entry : *shortcuts)
            {
                app_ids_to_app_names[entry.m_app_id] = entry.m_app_name;
                stream << Qt::endl << "  " << entry.m_app_id.getGameId() << " -> " << entry.m_app_name;
            }
        }
        else
        {
            stream << "Found no non-Steam shortcuts.";
        }

        qCInfo(lc::os).noquote() << buffer;
        return app_ids_to_app_names;
    }

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
