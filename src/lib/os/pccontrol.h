#pragma once

// system/Qt includes
#include <QtWidgets/QSystemTrayIcon>
#include <memory>

// local includes
#include "autostarthandlerinterface.h"
#include "cursorhandlerinterface.h"
#include "pcstatehandlerinterface.h"
#include "resolutionhandlerinterface.h"
#include "streamstatehandlerinterface.h"
#include "steamhandler.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class PcControl : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(PcControl)

public:
    explicit PcControl();
    ~PcControl() override = default;

    bool launchSteamApp(uint app_id);
    bool closeSteam(std::optional<uint> grace_period_in_sec);

    bool shutdownPC(uint grace_period_in_sec);
    bool restartPC(uint grace_period_in_sec);
    bool suspendPC(uint grace_period_in_sec);

    bool endStream();

    uint                getRunningApp() const;
    std::optional<uint> getTrackedUpdatingApp() const;
    bool                isSteamRunning() const;

    shared::StreamState getStreamState() const;
    shared::PcState     getPcState() const;

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

private:
    std::unique_ptr<AutoStartHandlerInterface>   m_auto_start_handler;
    std::unique_ptr<CursorHandlerInterface>      m_cursor_handler;
    std::unique_ptr<PcStateHandlerInterface>     m_pc_state_handler;
    std::unique_ptr<ResolutionHandlerInterface>  m_resolution_handler;
    std::unique_ptr<SteamHandler>                m_steam_handler;
    std::unique_ptr<StreamStateHandlerInterface> m_stream_state_handler;
};
}  // namespace os
