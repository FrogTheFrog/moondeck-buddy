#pragma once

// local includes
#include "../shared/trackedappdata.h"
#include "../steamregistryobserverinterface.h"
#include "registryfilewatcher.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class SteamRegistryObserver : public SteamRegistryObserverInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(SteamRegistryObserver)

public:
    explicit SteamRegistryObserver();
    ~SteamRegistryObserver() override = default;

    void startTrackingApp(uint app_id) override;
    void stopTrackingApp() override;

private slots:
    void slotRegistryChanged();

private:
    RegistryFileWatcher           m_watcher;
    QString                       m_steam_exec;
    uint                          m_pid{0};
    uint                          m_global_app_id{0};
    std::optional<TrackedAppData> m_tracked_app_data;
};
}  // namespace os
