#pragma once

// local includes
#include "appid.h"
#include "common/enums.h"
#include "steamprocesstracker.h"

namespace steam
{
class SteamAppWatcher : public QObject
{
    Q_OBJECT

public:
    explicit SteamAppWatcher(const SteamProcessTracker& process_tracker, const AppId& app_id);
    ~SteamAppWatcher() override;

    static std::optional<enums::AppState> getAppState(const SteamProcessTracker& process_tracker, const AppId& app_id);

    enums::AppState getAppState() const;
    const AppId&    getAppId() const;

private slots:
    void slotCheckState();

private:
    struct TrackingMetadata
    {
        // It is possible that the `steam_appid.txt` can override the AppId for non-Steam game, we need to take
        // this into account.
        AppId m_trackable_app_id;

        static std::optional<TrackingMetadata> fromAppId(const SteamLogTrackers&      log_trackers,
                                                         const std::filesystem::path& steam_dir, const AppId& app_id);
    };

    static enums::AppState getAppState(const SteamLogTrackers& log_trackers, const TrackingMetadata& metadata,
                                       enums::AppState prev_state);

    const SteamProcessTracker&      m_process_tracker;
    AppId                           m_app_id;
    std::optional<TrackingMetadata> m_metadata;

    enums::AppState m_current_state{enums::AppState::Stopped};
    QTimer          m_check_timer;
};
}  // namespace steam
