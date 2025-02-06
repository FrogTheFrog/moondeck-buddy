#pragma once

// system/Qt includes
#include <QtWidgets/QSystemTrayIcon>
#include <memory>

// local includes
#include "os/autostarthandler.h"
#include "os/pcstatehandler.h"
#include "os/resolutionhandler.h"
#include "os/steamhandler.h"

// forward declarations
namespace shared
{
class AppMetadata;
}  // namespace shared
namespace os
{
class AutoStartHandlerInterface;
class StreamStateHandlerInterface;
}  // namespace os

namespace os
{
class PcControl : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(PcControl)

public:
    explicit PcControl(const shared::AppMetadata& app_meta, const std::set<QString>& handled_displays,
                       QString registry_file_override);
    ~PcControl() override;

    bool launchSteamApp(uint app_id, bool force_big_picture);
    bool closeSteam(std::optional<uint> grace_period_in_sec);

    bool shutdownPC(uint grace_period_in_sec);
    bool restartPC(uint grace_period_in_sec);
    bool suspendPC(uint grace_period_in_sec, bool close_steam);
    bool hibernatePC(uint grace_period_in_sec, bool close_steam);

    bool endStream();

    uint                getRunningApp() const;
    std::optional<uint> getTrackedUpdatingApp() const;
    bool                isSteamRunning() const;

    enums::StreamState getStreamState() const;
    enums::PcState     getPcState() const;

    void setAutoStart(bool enable);
    bool isAutoStartEnabled() const;

    bool changeResolution(uint width, uint height);
    void restoreChangedResolution(bool force);

signals:
    void signalShowTrayMessage(const QString& title, const QString& message, QSystemTrayIcon::MessageIcon icon,
                               int millisecondsTimeoutHint);

private slots:
    void slotHandleSteamProcessStateChange();
    void slotHandleStreamStateChange();
    void slotAppTrackingHasEnded();

private:
    const shared::AppMetadata&                   m_app_meta;
    AutoStartHandler                             m_auto_start_handler;
    PcStateHandler                               m_pc_state_handler;
    ResolutionHandler                            m_resolution_handler;
    SteamHandler                                 m_steam_handler;
    std::unique_ptr<StreamStateHandlerInterface> m_stream_state_handler;
};
}  // namespace os
