#pragma once

// local includes
#include "os/steam/steamprocesstracker.h"
#include "shared/enums.h"

// forward declarations
namespace utils
{
class AppSettings;
}  // namespace utils
namespace os
{
class SteamAppWatcher;
}  // namespace os

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

    bool               launchSteam(bool big_picture_mode);
    enums::SteamUiMode getSteamUiMode() const;
    bool               close();

    std::optional<std::tuple<std::uint64_t, enums::AppState>>
         getAppData(const std::optional<std::uint64_t>& app_id) const;
    bool launchApp(std::uint64_t app_id);
    void clearSessionData();

    std::optional<std::map<std::uint64_t, QString>> getNonSteamAppData(std::uint64_t user_id) const;

signals:
    void signalSteamClosed();

private slots:
    void slotSteamProcessStateChanged();

private:
    struct SessionData
    {
        std::unique_ptr<SteamAppWatcher> m_steam_app_watcher;
    };

    const utils::AppSettings& m_app_settings;
    SteamProcessTracker       m_steam_process_tracker;
    SessionData               m_session_data;
};
}  // namespace os
