#pragma once

// system/Qt includes
#include <QString>
#include <QtNetwork/QSsl>
#include <set>

//---------------------------------------------------------------------------------------------------------------------

namespace utils
{
class AppSettings
{
    Q_DISABLE_COPY(AppSettings)

public:
    explicit AppSettings(const QString& filepath);
    virtual ~AppSettings() = default;

    quint16                  getPort() const;
    const QString&           getLoggingRules() const;
    const std::set<QString>& getHandledDisplays() const;
    const QString&           getSunshineAppsFilepath() const;
    bool                     getPreferHibernation() const;
    QSsl::SslProtocol        getSslProtocol() const;
    bool                     getForceBigPicture() const;
    bool                     getCloseSteamBeforeSleep() const;

private:
    bool parseSettingsFile(const QString& filepath);
    void saveDefaultFile(const QString& filepath) const;

    quint16           m_port;
    QString           m_logging_rules;
    std::set<QString> m_handled_displays;
    QString           m_sunshine_apps_filepath;
    bool              m_prefer_hibernation;
    QSsl::SslProtocol m_ssl_protocol;
    bool              m_force_big_picture;
    bool              m_close_steam_before_sleep;

public:
    // TODO: remove
    bool m_nvidia_reset_mouse_acceleration_after_stream_end_hack;
};
}  // namespace utils
