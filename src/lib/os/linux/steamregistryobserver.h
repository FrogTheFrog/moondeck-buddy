#pragma once

// local includes
#include "../shared/trackedappdata.h"
#include "../steamregistryobserverinterface.h"
#include "registryfilewatcher.h"
#include "steamprocesslistobserver.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class SteamRegistryObserver : public SteamRegistryObserverInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(SteamRegistryObserver)

public:
    explicit SteamRegistryObserver(QString registry_file_override, QString steam_binary_override);
    ~SteamRegistryObserver() override = default;

    void startAppObservation() override;
    void stopAppObservation() override;

    void startTrackingApp(uint app_id) override;
    void stopTrackingApp() override;

private slots:
    void slotRegistryChanged();

private:
    bool                          m_is_observing_apps{false};
    QTimer                        m_observation_delay;
    RegistryFileWatcher           m_watcher;
    QString                       m_steam_exec;
    uint                          m_pid{0};
    uint                          m_global_app_id{0};
    uint                          m_recheck_counter{0};
    std::optional<TrackedAppData> m_tracked_app_data;
    SteamProcessListObserver      m_process_list_observer;
};
}  // namespace os
