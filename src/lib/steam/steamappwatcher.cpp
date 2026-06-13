// header file include
#include "steam/steamappwatcher.h"

// system/Qt includes
#include <QFile>

// local includes
#include "common/loggingcategories.h"
#include "steam/shortcutsvdf.h"
#include "steam/steamprocesstracker.h"

namespace
{
std::optional<steam::AppId> tryFindAppIdOverrideForNonSteamGame(const steam::SteamLogTrackers& log_trackers,
                                                                const std::filesystem::path&   steam_dir,
                                                                const steam::AppId&            app_id)
{
    if (steam_dir.empty())
    {
        qCWarning(lc::steam) << "Steam directory is not available yet!";
        return std::nullopt;
    }

    const auto current_steam_id{log_trackers.getConnectionLog().getCurrentSteamId()};
    if (!current_steam_id)
    {
        qCWarning(lc::steam) << "User's SteamId is not available yet - cannot launch games until user logs in!";
        return std::nullopt;
    }

    const auto entries{steam::ShortcutsVdfEntry::scrapeShortcutsVdf(steam_dir, *current_steam_id)};
    if (!entries)
    {
        return std::nullopt;
    }

    const auto found_entry{std::ranges::find_if(*entries, [&app_id](const auto& entry)
                                                { return entry.m_app_id.getGameId() == app_id.getGameId(); })};
    if (found_entry == entries->end())
    {
        // The shortcuts VDF does not contain such an entry - fallback to the usual detection.
        qCWarning(lc::steam) << "shortcuts.vdf does not contain " << app_id.getGameId()
                             << "game id! Falling back to normal detection.";
        return app_id;
    }

    QFile file{std::filesystem::path{found_entry->m_start_dir.toStdString()} / "steam_appid.txt"};
    if (!file.exists())
    {
        qCInfo(lc::steam) << "AppId override file" << file.filesystemFileName().generic_string() << "does not exist.";
        return app_id;
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        qCWarning(lc::steam) << "file" << file.filesystemFileName().generic_string()
                             << "could not be opened! Falling back to normal detection.";
        return app_id;
    }

    // If it's gibberish, Steam will update it at some point
    return steam::AppId::fromString(file.readLine().trimmed());
}
}  // namespace

namespace steam
{
SteamAppWatcher::SteamAppWatcher(const SteamProcessTracker& process_tracker, const AppId& app_id)
    : m_process_tracker{process_tracker}
    , m_app_id{app_id}
    , m_metadata{std::nullopt}
{
    connect(&m_check_timer, &QTimer::timeout, this, &SteamAppWatcher::slotCheckState);

    m_check_timer.setInterval(500);
    m_check_timer.setSingleShot(true);

    qCInfo(lc::steam) << "Started watching AppID:" << m_app_id.getId();
    slotCheckState();
}

SteamAppWatcher::~SteamAppWatcher()
{
    qCInfo(lc::steam) << "Stopped watching AppID:" << m_app_id.getId();
}

std::optional<enums::AppState> SteamAppWatcher::getAppState(const SteamProcessTracker& process_tracker,
                                                            const AppId&               app_id)
{
    if (const auto* log_trackers{process_tracker.getSteamLogTrackers()})
    {
        if (const auto metadata{TrackingMetadata::fromAppId(*log_trackers, process_tracker.getSteamDir(), app_id)})
        {
            return getAppState(*log_trackers, *metadata, enums::AppState::Stopped);
        }
    }

    return std::nullopt;
}

enums::AppState SteamAppWatcher::getAppState() const
{
    return m_current_state;
}

const AppId& SteamAppWatcher::getAppId() const
{
    return m_app_id;
}

void SteamAppWatcher::slotCheckState()
{
    m_check_timer.stop();
    const auto auto_start_timer{qScopeGuard([this]() { m_check_timer.start(); })};

    auto new_state{enums::AppState::Stopped};
    if (const auto* log_trackers{m_process_tracker.getSteamLogTrackers()})
    {
        connect(log_trackers, &SteamLogTrackers::signalStateChanged, this, &SteamAppWatcher::slotCheckState,
                Qt::UniqueConnection);

        if (!m_metadata)
        {
            m_metadata = TrackingMetadata::fromAppId(*log_trackers, m_process_tracker.getSteamDir(), m_app_id);
            if (m_metadata && m_metadata->m_trackable_app_id != m_app_id)
            {
                qCInfo(lc::steam) << "[TRACKING] AppID override detected for non-Steam game. Mapping"
                                  << m_app_id.getId() << "->" << m_metadata->m_trackable_app_id.getId();
            }
        }

        if (m_metadata)
        {
            new_state = getAppState(*log_trackers, *m_metadata, m_current_state);
        }
    }

    if (new_state != m_current_state)
    {
        qCInfo(lc::steam) << "[TRACKING] New app state for AppID" << m_app_id.getId()
                          << "detected:" << enums::qEnumToString(m_current_state) << "->"
                          << enums::qEnumToString(new_state);
        m_current_state = new_state;
    }
}

std::optional<SteamAppWatcher::TrackingMetadata>
    SteamAppWatcher::TrackingMetadata::fromAppId(const SteamLogTrackers&      log_trackers,
                                                 const std::filesystem::path& steam_dir, const AppId& app_id)
{
    if (!app_id.isGameId())
    {
        return TrackingMetadata{app_id};
    }

    if (const auto non_steam_app_id{tryFindAppIdOverrideForNonSteamGame(log_trackers, steam_dir, app_id)})
    {
        return TrackingMetadata{*non_steam_app_id};
    }

    return std::nullopt;
}

enums::AppState SteamAppWatcher::getAppState(const SteamLogTrackers& log_trackers, const TrackingMetadata& metadata,
                                             const enums::AppState prev_state)
{
    auto       new_state{enums::AppState::Stopped};
    const auto content_state{log_trackers.getContentLog().getAppState(metadata.m_trackable_app_id)};

    if (content_state == SteamContentLogTracker::AppState::Updating
        || log_trackers.getShaderLog().isAppCompilingShaders(metadata.m_trackable_app_id))
    {
        new_state = enums::AppState::Updating;
    }
    else if (content_state == SteamContentLogTracker::AppState::Running)
    {
        new_state = enums::AppState::Running;
    }
    else if (log_trackers.getGameProcessLog().isAnyProcessRunning(metadata.m_trackable_app_id))
    {
        // Try to preserve the latest state from other logs, unless this is the only data available
        new_state = prev_state == enums::AppState::Stopped ? enums::AppState::Running : prev_state;
    }

    return new_state;
}
}  // namespace steam
