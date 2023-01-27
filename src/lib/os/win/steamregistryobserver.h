#pragma once

// local includes
#include "../shared/trackedappdata.h"
#include "../steamregistryobserverinterface.h"
#include "regkey.h"

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

    void startAppObservation() override;
    void stopAppObservation() override;

    void startTrackingApp(uint app_id) override;
    void stopTrackingApp() override;

private slots:
    void slotRegistryChanged(const QMap<QString, QVariant>& changed_values);

private:
    RegKey m_global_reg_key;
    RegKey m_steam_exec_reg_key;
    RegKey m_process_reg_key;
    RegKey m_app_reg_key;

    bool                          m_is_observing_apps{false};
    QTimer                        m_observation_delay;
    QString                       m_steam_exec;
    uint                          m_pid{0};
    uint                          m_global_app_id{0};
    std::optional<TrackedAppData> m_tracked_app_data;
};
}  // namespace os
