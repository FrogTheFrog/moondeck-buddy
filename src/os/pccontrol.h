#pragma once

// system/Qt includes
#include <memory>

// local includes
#include "pccontrolinterface.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class PcControl : public PcControlInterface
{
    Q_DISABLE_COPY(PcControl)

public:
    explicit PcControl(QString app_name);
    ~PcControl() override = default;

    void launchSteamApp(uint app_id) override;
    void exitSteam(std::optional<uint> grace_period_in_sec) override;

    void shutdownPC(uint grace_period_in_sec) override;
    void restartPC(uint grace_period_in_sec) override;

    uint getRunningApp() const override;
    bool isSteamRunning() const override;

    void setAutoStart(bool enable) override;
    bool isAutoStartEnabled() const override;

private:
    std::unique_ptr<PcControlInterface> m_impl;
};
}  // namespace os
