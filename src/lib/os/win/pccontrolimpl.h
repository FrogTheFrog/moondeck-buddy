#pragma once

// system/Qt includes
#include <QTimer>

// local includes
#include "../pccontrolinterface.h"
#include "resolutionhandler.h"
#include "steamhandler.h"
#include "streamstatehandler.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class PcControlImpl : public PcControlInterface
{
    Q_DISABLE_COPY(PcControlImpl)

public:
    explicit PcControlImpl(QString app_name);
    ~PcControlImpl() override = default;

    void launchSteamApp(uint app_id) override;
    void exitSteam(std::optional<uint> grace_period_in_sec) override;

    void shutdownPC(uint grace_period_in_sec) override;
    void restartPC(uint grace_period_in_sec) override;

    uint                getRunningApp() const override;
    std::optional<uint> isLastLaunchedAppUpdating() const override;
    bool                isSteamRunning() const override;

    shared::StreamState getStreamState() const override;

    void setAutoStart(bool enable) override;
    bool isAutoStartEnabled() const override;

    void changeResolution(uint width, uint height, bool immediate) override;
    void abortPendingResolutionChange() override;
    void restoreChangedResolution() override;

private slots:
    void slotHandleSteamStart();
    void slotHandleSteamExit();

    void slotHandleStreamStateChange();

private:
    QString            m_app_name;
    ResolutionHandler  m_resolution_handler;
    SteamHandler       m_steam_handler;
    StreamStateHandler m_stream_state_handler;
    QTimer             m_pc_delay_timer;
};
}  // namespace os