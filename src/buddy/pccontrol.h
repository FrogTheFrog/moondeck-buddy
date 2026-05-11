#pragma once

// system/Qt includes
#include <QtWidgets/QSystemTrayIcon>

// local includes
#include "os/autostarthandler.h"
#include "os/pcstatehandler.h"
#include "steam/steamhandler.h"
#include "streamstatehandler.h"
#include "utils/shmserialization.h"

class PcControl : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(PcControl)

public:
    explicit PcControl(const common::AppSettings& app_settings);
    ~PcControl() override;

    bool               launchSteam(bool big_picture_mode, const QString& username);
    enums::SteamUiMode getSteamUiMode() const;
    bool               closeSteam(bool keep_stream_alive);
    bool               closeSteamBigPictureMode();

    bool launchSteamApp(const steam::AppId& app_id);
    std::optional<std::tuple<steam::AppId, enums::AppState>>
         getAppData(const std::optional<steam::AppId>& app_id) const;
    bool clearAppData();

    std::optional<std::map<steam::AppId, QString>> getNonSteamAppData(const steam::SteamId& user_id) const;
    std::optional<steam::SteamId>                  getCurrentUserId() const;

    bool shutdownPC(uint delay_in_seconds);
    bool restartPC(uint delay_in_seconds);
    bool suspendOrHibernatePC(uint delay_in_seconds);
    bool endStream();

    enums::StreamState getStreamState() const;
    enums::PcState     getPcState() const;

    void setAutoStart(bool enable);
    bool isAutoStartEnabled() const;

    bool isServiceSupported() const;
    bool restartIntoService();

signals:
    void signalShowTrayMessage(const QString& title, const QString& message, QSystemTrayIcon::MessageIcon icon,
                               int milliseconds_timeout_hint);

private slots:
    void slotHandleSteamClosed();
    void slotHandleStreamStateChange();

private:
    const common::AppSettings& m_app_settings;
    os::AutoStartHandler       m_auto_start_handler;
    os::PcStateHandler         m_pc_state_handler;
    steam::SteamHandler        m_steam_handler;
    StreamStateHandler         m_stream_state_handler;

    utils::ShmDeserializer m_shared_env_reader;
    QMap<QString, QString> m_cached_env;

    bool m_keep_stream_alive{false};
};
