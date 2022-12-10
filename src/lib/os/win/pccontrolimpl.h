#pragma once

// system/Qt includes
#include <QTimer>

// local includes
#include "../pccontrolinterface.h"
#include "messagequeue.h"
#include "processtracker.h"
#include "regkey.h"

// forward declarations
namespace os
{
class WinEventFilter;
}

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

    void setAutoStart(bool enable) override;
    bool isAutoStartEnabled() const override;

    void changeResolution(uint width, uint height, bool immediate) override;
    void restoreChangedResolution() override;

private slots:
    void slotSteamProcessStateChanged();
    void slotHandleRegKeyChanges(const QMap<QString, QVariant>& changed_values);
    void slotHandleExitTimeout();

    // NOLINTNEXTLINE(readability-redundant-access-specifiers)
private:
    struct LaunchedAppData
    {
        uint m_app_id;
        bool m_is_running;
        bool m_is_updating;
    };

    struct PendingResolutionChange
    {
        uint m_width;
        uint m_height;
    };

    RegKey                                 m_reg_key;
    std::unique_ptr<RegKey>                m_app_reg_key;
    ProcessTracker                         m_process_tracker;
    MessageQueue                           m_message_queue;
    QString                                m_steam_exec;
    QString                                m_app_name;
    QTimer                                 m_exit_timer;
    QTimer                                 m_pc_delay_timer;
    std::optional<uint>                    m_global_app_id;
    std::optional<LaunchedAppData>         m_launched_app;
    std::optional<PendingResolutionChange> m_pending_resolution_change;
};
}  // namespace os