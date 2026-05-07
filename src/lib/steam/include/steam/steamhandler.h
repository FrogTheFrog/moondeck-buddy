#pragma once

// local includes
#include "common/enums.h"
#include "steamcommandproxy.h"
#include "steamid.h"
#include "steamprocesstracker.h"

// forward declarations
namespace utils
{
class AppSettings;
}  // namespace utils
namespace steam
{
class SteamAppWatcher;
}  // namespace steam

namespace steam
{
class SteamHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SteamHandler)

public:
    explicit SteamHandler(const utils::AppSettings& app_settings);
    ~SteamHandler() override;

    bool               launchSteam(bool big_picture_mode, const QMap<QString, QString>& env_overrides);
    enums::SteamUiMode getSteamUiMode() const;
    bool               close();
    bool               closeBigPictureMode();

    std::optional<std::tuple<AppId, enums::AppState>> getAppData(const std::optional<AppId>& app_id) const;
    bool launchApp(const AppId& app_id, const QMap<QString, QString>& env_overrides);
    void clearSessionData();

    std::optional<std::map<AppId, QString>> getNonSteamAppData(const SteamId& user_id) const;

signals:
    void signalSteamClosed();

private slots:
    void slotSteamProcessStateChanged();

private:
    struct SessionData
    {
        std::unique_ptr<SteamAppWatcher> m_steam_app_watcher;
    };

    SteamCommandProxy   m_command_proxy;
    SteamProcessTracker m_steam_process_tracker;
    SessionData         m_session_data;
};
}  // namespace steam
