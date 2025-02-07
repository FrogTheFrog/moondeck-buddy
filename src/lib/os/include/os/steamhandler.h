#pragma once

// local includes
#include "os/shared/trackedappdata.h"
#include "os/steam/steamprocesstracker.h"

// forward declarations
namespace os
{
class SteamRegistryObserverInterface;
}

namespace os
{
class SteamHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SteamHandler)

public:
    explicit SteamHandler(std::function<QString()>                        m_steam_exec_path_getter,
                          std::unique_ptr<SteamProcessTracker>            steam_process_tracker,
                          std::unique_ptr<SteamRegistryObserverInterface> registry_observer);
    ~SteamHandler() override;

    bool isRunning() const;
    bool isRunningNow();
    bool close(std::optional<uint> grace_period_in_sec);

    bool                launchApp(uint app_id, bool force_big_picture);
    uint                getRunningApp() const;
    std::optional<uint> getTrackedActiveApp() const;
    std::optional<uint> getTrackedUpdatingApp() const;
    void                clearTrackedApp();

signals:
    void signalProcessStateChanged();

private slots:
    void slotSteamProcessStateChanged();
    void slotGlobalAppId(uint app_id);
    void slotTrackedAppIsRunning(bool state);
    void slotTrackedAppIsUpdating(bool state);
    void slotTerminateSteam();

private:
    std::function<QString()>                        m_steam_exec_path_getter;
    std::unique_ptr<SteamProcessTracker>            m_steam_process_tracker;
    std::unique_ptr<SteamRegistryObserverInterface> m_registry_observer;

    uint                          m_global_app_id{0};
    std::optional<TrackedAppData> m_tracked_app;
    QTimer                        m_steam_close_timer;
};
}  // namespace os
