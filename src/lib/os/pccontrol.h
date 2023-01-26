#pragma once

// system/Qt includes
#include <QtWidgets/QSystemTrayIcon>
#include <memory>

// local includes
#include "pcstatehandler.h"
#include "resolutionhandler.h"
#include "steamhandler.h"

// forward declarations
namespace shared
{
class AppMetadata;
}  // namespace shared
namespace os
{
class AutoStartHandlerInterface;
class CursorHandlerInterface;
class StreamStateHandlerInterface;
}  // namespace os

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class PcControl : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(PcControl)

public:
    explicit PcControl(const shared::AppMetadata& app_meta, const std::set<QString>& handled_displays);
    ~PcControl() override;

    bool launchSteamApp(uint app_id);
    bool closeSteam(std::optional<uint> grace_period_in_sec);

    bool shutdownPC(uint grace_period_in_sec);
    bool restartPC(uint grace_period_in_sec);
    bool suspendPC(uint grace_period_in_sec);

    bool endStream();

    uint                getRunningApp() const;
    std::optional<uint> getTrackedUpdatingApp() const;
    bool                isSteamRunning() const;

    enums::StreamState getStreamState() const;
    enums::PcState     getPcState() const;

    void setAutoStart(bool enable);
    bool isAutoStartEnabled() const;

    bool changeResolution(uint width, uint height);
    void restoreChangedResolution();

signals:
    void signalShowTrayMessage(const QString& title, const QString& message, QSystemTrayIcon::MessageIcon icon,
                               int millisecondsTimeoutHint);

private slots:
    void slotHandleSteamProcessStateChange();
    void slotHandleStreamStateChange();
    void slotAppTrackingHasEnded();

private:
    const shared::AppMetadata&                   m_app_meta;
    std::unique_ptr<AutoStartHandlerInterface>   m_auto_start_handler;
    std::unique_ptr<CursorHandlerInterface>      m_cursor_handler;
    PcStateHandler                               m_pc_state_handler;
    ResolutionHandler                            m_resolution_handler;
    SteamHandler                                 m_steam_handler;
    std::unique_ptr<StreamStateHandlerInterface> m_stream_state_handler;
};
}  // namespace os
