#pragma once

// system/Qt includes
#include <QObject>

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class SteamHandlerInterface : public QObject
{
    Q_OBJECT

public:
    ~SteamHandlerInterface() override = default;

    virtual bool isRunning() const                              = 0;
    virtual bool isRunningNow()                                 = 0;
    virtual bool close(std::optional<uint> grace_period_in_sec) = 0;

    virtual bool                launchApp(uint app_id)        = 0;
    virtual uint                getRunningApp() const         = 0;
    virtual std::optional<uint> getTrackedUpdatingApp() const = 0;

signals:
    void signalProcessStateChanged();
};
}  // namespace os
