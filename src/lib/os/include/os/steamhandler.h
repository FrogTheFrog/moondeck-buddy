#pragma once

// local includes
#include "os/steam/steamprocesstracker.h"

// forward declarations
namespace utils
{
class AppSettings;
}  // namespace utils
namespace os
{
class SteamLauncher;
}

namespace os
{
class SteamHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SteamHandler)

public:
    explicit SteamHandler(const utils::AppSettings&                      app_settings,
                          std::unique_ptr<NativeProcessHandlerInterface> process_handler_interface);
    ~SteamHandler() override;

    bool isSteamReady() const;
    bool close();

    bool launchApp(uint app_id);
    void clearSessionData();

signals:
    void signalSteamClosed();

private slots:
    void slotSteamProcessStateChanged();
    void slotSteamLaunchFinished(const QString& steam_exec, uint app_id, bool success);

private:
    struct SessionData
    {
        std::unique_ptr<SteamLauncher> m_steam_launcher;
    };

    const utils::AppSettings& m_app_settings;
    SteamProcessTracker       m_steam_process_tracker;
    SessionData               m_session_data;
};
}  // namespace os
