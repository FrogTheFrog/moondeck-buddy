#pragma once

// system/Qt includes
#include <QObject>

// local includes
#include "appid.h"

// forward declarations
namespace utils
{
class AppSettings;
}  // namespace utils

namespace steam
{
class SteamCommandProxy final
{
    Q_DISABLE_COPY(SteamCommandProxy)

public:
    explicit SteamCommandProxy(const utils::AppSettings& app_settings);
    ~SteamCommandProxy() = default;

    bool canExecuteCommands() const;

    bool launchSteam(bool big_picture_mode, const QMap<QString, QString>& env_overrides);
    bool launchApp(const AppId& app_id, const QMap<QString, QString>& env_overrides);

    bool close();
    bool closeBigPictureMode();

private:
    const utils::AppSettings& m_app_settings;
};
}  // namespace steam
