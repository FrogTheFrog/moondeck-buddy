#pragma once

// system/Qt includes
#include <QtWidgets/QSystemTrayIcon>
#include <memory>

// local includes
#include "os/autostarthandler.h"
#include "os/pcstatehandler.h"
#include "os/steamhandler.h"

// forward declarations
namespace utils
{
class AppSettings;
}  // namespace utils
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
    explicit PcControl(const utils::AppSettings& app_settings);
    ~PcControl() override;

    bool isSteamReady() const;
    bool closeSteam();

    bool                                             launchSteamApp(uint app_id);
    std::optional<std::tuple<uint, enums::AppState>> getAppData() const;

    bool shutdownPC();
    bool restartPC();
    bool suspendOrHibernatePC();
    bool endStream();

    enums::StreamState getStreamState() const;
    enums::PcState     getPcState() const;

    void setAutoStart(bool enable);
    bool isAutoStartEnabled() const;

signals:
    void signalShowTrayMessage(const QString& title, const QString& message, QSystemTrayIcon::MessageIcon icon,
                               int milliseconds_timeout_hint);

private slots:
    void slotHandleSteamClosed();
    void slotHandleStreamStateChange();

private:
    const utils::AppSettings&                    m_app_settings;
    AutoStartHandler                             m_auto_start_handler;
    PcStateHandler                               m_pc_state_handler;
    SteamHandler                                 m_steam_handler;
    std::unique_ptr<StreamStateHandlerInterface> m_stream_state_handler;
};
}  // namespace os
