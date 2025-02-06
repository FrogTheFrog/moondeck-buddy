#pragma once

// local includes
#include "os/linux/registryfilewatcher.h"
#include "os/linux/steamprocesslistobserver.h"
#include "os/shared/steamregistryobserverinterface.h"
#include "os/shared/trackedappdata.h"

namespace os
{
class SteamRegistryObserver : public SteamRegistryObserverInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(SteamRegistryObserver)

public:
    explicit SteamRegistryObserver(QString registry_file_override);
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
    uint                          m_pid{0};
    uint                          m_global_app_id{0};
    uint                          m_recheck_counter{0};
    std::optional<TrackedAppData> m_tracked_app_data;
    SteamProcessListObserver      m_process_list_observer;
};
}  // namespace os
