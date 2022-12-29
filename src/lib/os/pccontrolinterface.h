#pragma once

// system/Qt includes
#include <QObject>
#include <QtWidgets/QSystemTrayIcon>

// local includes
#include "shared/enums.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class PcControlInterface : public QObject
{
    Q_OBJECT

public:
    ~PcControlInterface() override = default;

    virtual bool launchSteamApp(uint app_id)                         = 0;
    virtual bool closeSteam(std::optional<uint> grace_period_in_sec) = 0;

    virtual bool shutdownPC(uint grace_period_in_sec) = 0;
    virtual bool restartPC(uint grace_period_in_sec)  = 0;
    virtual bool suspendPC(uint grace_period_in_sec)  = 0;

    virtual uint                getRunningApp() const         = 0;
    virtual std::optional<uint> getTrackedUpdatingApp() const = 0;
    virtual bool                isSteamRunning() const        = 0;

    virtual shared::StreamState getStreamState() const = 0;
    virtual shared::PcState     getPcState() const     = 0;

    virtual void setAutoStart(bool enable)  = 0;
    virtual bool isAutoStartEnabled() const = 0;

    virtual bool changeResolution(uint width, uint height, bool immediate) = 0;
    virtual void abortPendingResolutionChange()                            = 0;
    virtual void restoreChangedResolution()                                = 0;

signals:
    void signalShowTrayMessage(const QString& title, const QString& message, QSystemTrayIcon::MessageIcon icon,
                               int millisecondsTimeoutHint);
};
}  // namespace os
