#pragma once

// system/Qt includes
#include <QTimer>

// local includes
#include "../pccontrolinterface.h"
#include "autostarthandler.h"
#include "pcstatehandler.h"
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
    explicit PcControlImpl();
    ~PcControlImpl() override = default;

    bool launchSteamApp(uint app_id) override;
    bool closeSteam(std::optional<uint> grace_period_in_sec) override;

    bool shutdownPC(uint grace_period_in_sec) override;
    bool restartPC(uint grace_period_in_sec) override;

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

private slots:
    void slotHandleSteamStateChange();
    void slotHandleStreamStateChange();

private:
    std::shared_ptr<ProcessEnumerator> m_enumerator;

    AutoStartHandler   m_auto_start_handler;
    PcStateHandler     m_pc_state_handler;
    ResolutionHandler  m_resolution_handler;
    SteamHandler       m_steam_handler;
    StreamStateHandler m_stream_state_handler;
};
}  // namespace os