#pragma once

// local includes
#include "shared/appid.h"
#include "shared/enums.h"
#include "steamprocesstracker.h"

namespace os
{
class SteamAppWatcher : public QObject
{
    Q_OBJECT

public:
    explicit SteamAppWatcher(const SteamProcessTracker& process_tracker, const shared::AppId& app_id);
    ~SteamAppWatcher() override;

    static std::optional<enums::AppState> getAppState(const SteamProcessTracker& process_tracker,
                                                      const shared::AppId&       app_id);

    enums::AppState      getAppState() const;
    const shared::AppId& getAppId() const;

private slots:
    void slotCheckState();

private:
    struct TrackingMetadata
    {
        // It is possible that the `steam_appid.txt` can override the AppId for non-Steam game, we need to take
        // this into account.
        shared::AppId m_trackable_app_id;

        static std::optional<TrackingMetadata> fromAppId(const SteamProcessTracker::LogTrackers& log_trackers,
                                                         const std::filesystem::path&            steam_dir,
                                                         const shared::AppId&                    app_id);
    };

    static enums::AppState getAppState(const SteamProcessTracker::LogTrackers& log_trackers,
                                       const TrackingMetadata& metadata, enums::AppState prev_state);

    const SteamProcessTracker&      m_process_tracker;
    shared::AppId                   m_app_id;
    std::optional<TrackingMetadata> m_metadata;

    enums::AppState m_current_state{enums::AppState::Stopped};
    QTimer          m_check_timer;
    uint            m_delay_counter{0};
};
}  // namespace os
