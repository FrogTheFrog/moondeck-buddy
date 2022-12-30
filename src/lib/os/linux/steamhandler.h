#pragma once

// system/Qt includes
#include <QObject>

// local includes
#include "../steamhandlerinterface.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class SteamHandler : public SteamHandlerInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(SteamHandler)

public:
    explicit SteamHandler()  = default;
    ~SteamHandler() override = default;

    bool isRunning() const override;
    bool isRunningNow() override;
    bool close(std::optional<uint> grace_period_in_sec) override;

    bool                launchApp(uint app_id, const QStringList& steam_args) override;
    uint                getRunningApp() const override;
    std::optional<uint> getTrackedUpdatingApp() const override;
};
}  // namespace os
