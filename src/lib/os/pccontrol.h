#pragma once

// system/Qt includes
#include <QCoreApplication>
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
    explicit PcControl();
    ~PcControl() override = default;

    bool launchSteamApp(uint app_id) override;
    bool closeSteam(std::optional<uint> grace_period_in_sec) override;

    bool shutdownPC(uint grace_period_in_sec) override;
    bool restartPC(uint grace_period_in_sec) override;
    bool suspendPC(uint grace_period_in_sec) override;

    uint                getRunningApp() const override;
    std::optional<uint> getTrackedUpdatingApp() const override;
    bool                isSteamRunning() const override;

    shared::StreamState getStreamState() const override;
    shared::PcState     getPcState() const override;

    void setAutoStart(bool enable) override;
    bool isAutoStartEnabled() const override;

    bool changeResolution(uint width, uint height, bool immediate) override;
    void abortPendingResolutionChange() override;
    void restoreChangedResolution() override;

private:
    std::unique_ptr<PcControlInterface> m_impl;
};
}  // namespace os
