#pragma once

// system/Qt includes
#include <QObject>

namespace os
{
class SteamRegistryObserverInterface : public QObject
{
    Q_OBJECT

public:
    ~SteamRegistryObserverInterface() override = default;

    virtual void startAppObservation() = 0;
    virtual void stopAppObservation()  = 0;

    virtual void startTrackingApp(uint app_id) = 0;
    virtual void stopTrackingApp()             = 0;

signals:
    void signalSteamExecPath(const QString& path);
    void signalSteamPID(uint pid);
    void signalGlobalAppId(uint app_id);
    void signalTrackedAppIsRunning(bool state);
    void signalTrackedAppIsUpdating(bool state);
};
}  // namespace os
