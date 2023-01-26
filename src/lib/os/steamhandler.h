#pragma once

// system/Qt includes
#include <QTimer>

// local includes
#include "processhandler.h"
#include "shared/trackedappdata.h"
#include "steamregistryobserverinterface.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class SteamHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SteamHandler)

public:
    explicit SteamHandler(std::unique_ptr<ProcessHandler>                 process_handler,
                          std::unique_ptr<SteamRegistryObserverInterface> registry_observer);
    ~SteamHandler() override = default;

    bool isRunning() const;
    bool isRunningNow();
    bool close(std::optional<uint> grace_period_in_sec);

    bool                launchApp(uint app_id);
    uint                getRunningApp() const;
    std::optional<uint> getTrackedActiveApp() const;
    std::optional<uint> getTrackedUpdatingApp() const;

signals:
    void signalProcessStateChanged();
    void signalAppTrackingHasEnded();

private slots:
    void slotSteamProcessDied();
    void slotSteamExecPath(const QString& path);
    void slotSteamPID(uint pid);
    void slotGlobalAppId(uint app_id);
    void slotTrackedAppIsRunning(bool state);
    void slotTrackedAppIsUpdating(bool state);
    void slotTerminateSteam();

private:
    void clearTrackedApp();

    std::unique_ptr<ProcessHandler>                 m_process_handler;
    std::unique_ptr<SteamRegistryObserverInterface> m_registry_observer;

    QString                       m_steam_exec_path;
    uint                          m_global_app_id{0};
    std::optional<TrackedAppData> m_tracked_app;
    QTimer                        m_steam_close_timer;
};
}  // namespace os
