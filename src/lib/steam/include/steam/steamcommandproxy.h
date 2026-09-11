#pragma once

// local includes
#include "appid.h"

// forward declarations
namespace common
{
struct AppSettings;
}  // namespace common

namespace steam
{
class SteamCommandProxy final
{
    Q_DISABLE_COPY(SteamCommandProxy)

public:
    explicit SteamCommandProxy(const common::AppSettings& app_settings);
    ~SteamCommandProxy() = default;

    bool canExecuteCommands() const;

    bool launchSteam(bool big_picture_mode, const QString& username, const QMap<QString, QString>& env_overrides);
    bool launchApp(const AppId& app_id, const QMap<QString, QString>& env_overrides);

    bool close();
    bool closeBigPictureMode();

private:
    const common::AppSettings& m_app_settings;
};
}  // namespace steam
