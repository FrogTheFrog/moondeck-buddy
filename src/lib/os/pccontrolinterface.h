#pragma once

// system/Qt includes
#include <QObject>

// local includes
#include "shared/pcstate.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class PcControlInterface : public QObject
{
    Q_OBJECT

public:
    ~PcControlInterface() override = default;

    virtual void launchSteamApp(uint app_id)                        = 0;
    virtual void exitSteam(std::optional<uint> grace_period_in_sec) = 0;

    virtual void shutdownPC(uint grace_period_in_sec) = 0;
    virtual void restartPC(uint grace_period_in_sec)  = 0;

    virtual uint                getRunningApp() const             = 0;
    virtual std::optional<uint> isLastLaunchedAppUpdating() const = 0;
    virtual bool                isSteamRunning() const            = 0;

    virtual void setAutoStart(bool enable)  = 0;
    virtual bool isAutoStartEnabled() const = 0;

    virtual void changeResolution(uint width, uint height, bool immediate) = 0;
    virtual void restoreChangedResolution()                                = 0;

signals:
    void signalPcStateChanged(shared::PcState state);
};
}  // namespace os
