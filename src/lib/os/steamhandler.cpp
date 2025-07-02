// header file include
#include "os/steamhandler.h"

// system/Qt includes
#include <QFile>
#include <QProcess>

// local includes
#include "os/shared/nativeprocesshandlerinterface.h"
#include "os/steam/steamappwatcher.h"
#include "shared/loggingcategories.h"
#include "utils/appsettings.h"
#include "utils/envsharedmemory.h"

namespace
{
bool executeDetached(const QString& steam_exec, const QStringList& args, const QMap<QString, QString>& envVars = {})
{
    QProcess steam_process;
    steam_process.setStandardOutputFile(QProcess::nullDevice());
    steam_process.setStandardErrorFile(QProcess::nullDevice());
    steam_process.setProgram(steam_exec);
    steam_process.setArguments(args);

    // Apply environment variables if provided
    if (!envVars.isEmpty())
    {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        for (auto it = envVars.constBegin(); it != envVars.constEnd(); ++it)
        {
            env.insert(it.key(), it.value());
            qCDebug(lc::os) << "Setting environment variable for game launch:" << it.key() << "=" << it.value();
        }
        steam_process.setProcessEnvironment(env);
        qCInfo(lc::os) << "Applied" << envVars.size() << "environment variables to Steam process";
    }

    return steam_process.startDetached();
}

// This is a very "son, we have a parser at home" kind of parser, very basic, but gets the job done...
std::optional<std::map<std::uint64_t, QString>> scrapeShortcutsVdf(const QByteArray& contents)
{
    const auto index_of_insensitive{
        [](const QByteArray& data, const QByteArrayView view, qsizetype from) -> qsizetype
        {
            const auto needle{view.at(0)};
            while (true)
            {
                from = data.indexOf(needle, from);
                if (from == -1)
                {
                    break;
                }

                if (const char* needle_ptr{data.data() + from};
                    qstrnicmp(needle_ptr, view.data(), qMin(data.size() - from, view.size())) == 0)
                {
                    break;
                }
                ++from;
            }

            return from;
        }};

    std::vector<std::uint32_t> app_ids;
    {
        constexpr QByteArrayView app_id_view("\x02"
                                             "appid"
                                             "\x00");
        qsizetype                from{0};
        while (true)
        {
            from = index_of_insensitive(contents, app_id_view, from);
            if (from == -1)
            {
                break;
            }
            from += app_id_view.size() + 1;

            if (from + 4 > contents.size())
            {
                qCWarning(lc::os) << "Out of range error while scraping shortcuts.vdf for appid!";
                return std::nullopt;
            }

            const std::uint32_t app_id{
                static_cast<std::uint32_t>(static_cast<std::uint8_t>(contents.at(from)))
                | static_cast<std::uint32_t>(static_cast<std::uint8_t>(contents.at(from + 1))) << 8U
                | static_cast<std::uint32_t>(static_cast<std::uint8_t>(contents.at(from + 2))) << 16U
                | static_cast<std::uint32_t>(static_cast<std::uint8_t>(contents.at(from + 3))) << 24U};
            app_ids.emplace_back(app_id);
        }
    }

    std::vector<QString> app_names;
    {
        constexpr QByteArrayView app_name_view("\x01"
                                               "appname"
                                               "\x00");
        qsizetype                from{0};
        while (true)
        {
            from = index_of_insensitive(contents, app_name_view, from);
            if (from == -1)
            {
                break;
            }
            from += app_name_view.size() + 1;

            const auto to = contents.indexOf('\0', from);
            if (to == -1)
            {
                qCWarning(lc::os) << "Out of range error while scraping shortcuts.vdf for appname!";
                return std::nullopt;
            }

            app_names.emplace_back(QString::fromUtf8(contents.data() + from, to - from));
        }
    }

    if (app_names.size() != app_ids.size())
    {
        qCWarning(lc::os) << "Failed to scrape shortcuts.vdf - app name and id list size mismatch!";
        return std::nullopt;
    }

    std::map<std::uint64_t, QString> data;
    for (std::size_t i{0}; i < app_ids.size(); ++i)
    {
        const auto game_id{static_cast<std::uint64_t>(app_ids[i]) << 32U | 0x02000000U};
        data[game_id] = app_names[i];
    }

    return data;
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
        // Retrieve environment variables from shared memory set by Stream application
        utils::EnvSharedMemory envMemory;
        QMap<QString, QString> envVars = envMemory.retrieveEnvironment();
        
        if (!envVars.isEmpty())
        {
            qCInfo(lc::os) << "Using" << envVars.size() << "environment variables from Stream for Steam launch:" << envVars.keys();
        }
        else
        {
            qCDebug(lc::os) << "No environment variables available from Stream - launching Steam with system environment";
        }
        
        if (!executeDetached(exec_path, big_picture_mode ? QStringList{"steam://open/bigpicture"} : QStringList{}, envVars))
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

std::optional<std::tuple<std::uint64_t, enums::AppState>>
    SteamHandler::getAppData(const std::optional<std::uint64_t>& app_id) const
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

    if (const auto app_data{getAppData(std::nullopt)}; app_data && std::get<std::uint64_t>(*app_data) != app_id)
    {
        qCWarning(lc::os) << "Buddy is already tracking app id: " << std::get<std::uint64_t>(*app_data);
        return false;
    }

    m_steam_process_tracker.slotCheckState();
    if (getSteamUiMode() == enums::SteamUiMode::Unknown)
    {
        qCWarning(lc::os) << "Steam is not running or has not reached a stable state yet!";
        return false;
    }

    const bool is_app_running{
        SteamAppWatcher::getAppState(m_steam_process_tracker, app_id).value_or(enums::AppState::Stopped)
        != enums::AppState::Stopped};
    
    if (!is_app_running)
    {
        // Retrieve environment variables from shared memory set by Stream application
        utils::EnvSharedMemory envMemory;
        QMap<QString, QString> envVars = envMemory.retrieveEnvironment();
        
        if (!envVars.isEmpty())
        {
            qCInfo(lc::os) << "Using" << envVars.size() << "environment variables from Stream for game launch:" << envVars.keys();
        }
        else
        {
            qCDebug(lc::os) << "No environment variables available from Stream - launching with system environment";
        }
        
        if (!executeDetached(exec_path, QStringList{"steam://rungameid/" + QString::number(app_id)}, envVars))
        {
            qCWarning(lc::os) << "Failed to perform app launch for AppID: " << app_id;
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

std::optional<std::map<std::uint64_t, QString>> SteamHandler::getNonSteamAppData(const std::uint64_t user_id) const
{
    const auto& steam_dir{m_steam_process_tracker.getSteamDir()};
    if (steam_dir.empty())
    {
        qCWarning(lc::os) << "Steam directory is not available yet!";
        return std::nullopt;
    }

    const auto user_dir_id{[user_id]() -> QString
                           {
                               // See https://developer.valvesoftware.com/wiki/SteamID for how to get SteamID3
                               const uint64_t id_number{user_id & 0x1U};
                               const uint64_t account_number{(user_id & 0xFFFFFFFE) >> 1U};
                               return QString::number((account_number * 2) + id_number);
                           }()};
    const auto shortcuts_file{steam_dir / "userdata" / user_dir_id.toStdString() / "config" / "shortcuts.vdf"};
    qInfo(lc::os) << "Mapped user id to shortcuts file:" << user_id << "->" << shortcuts_file.generic_string();

    QFile file{shortcuts_file};
    if (!file.exists())
    {
        qCWarning(lc::os) << "file" << shortcuts_file.generic_string() << "does not exist!";
        return std::nullopt;
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        qCWarning(lc::os) << "file" << shortcuts_file.generic_string() << "could not be opened!";
        return std::nullopt;
    }

    const auto shortcuts{scrapeShortcutsVdf(file.readAll())};
    if (shortcuts)
    {
        QString     buffer;
        QTextStream stream(&buffer);
        if (!shortcuts->empty())
        {
            stream << "Found " << shortcuts->size() << " non-Steam shortcut(-s):";
            for (const auto& [app_id, app_name] : *shortcuts)
            {
                stream << Qt::endl << "  " << app_id << " -> " << app_name;
            }
        }
        else
        {
            stream << "Found no non-Steam shortcuts.";
        }

        qCInfo(lc::os).noquote() << buffer;
    }

    return shortcuts;
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
