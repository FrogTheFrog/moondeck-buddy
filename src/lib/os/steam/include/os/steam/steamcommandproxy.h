#pragma once

// system/Qt includes
#include <QObject>

// forward declarations
namespace utils
{
class AppSettings;
}  // namespace utils

namespace os
{
class SteamCommandProxy final
{
    Q_DISABLE_COPY(SteamCommandProxy)

public:
    explicit SteamCommandProxy(const utils::AppSettings& app_settings);
    ~SteamCommandProxy() = default;

    bool canExecuteCommands() const;

    bool launchSteam(bool big_picture_mode, const QMap<QString, QString>& env_overrides);
    bool launchApp(std::uint64_t app_id, const QMap<QString, QString>& env_overrides);

    bool close();
    bool closeBigPictureMode();

private:
    const utils::AppSettings& m_app_settings;
};
}  // namespace os
