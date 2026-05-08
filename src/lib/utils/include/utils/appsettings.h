#pragma once

// system/Qt includes
#include <QRegularExpression>
#include <QString>
#include <QtNetwork/QSsl>

// local includes
#include "common/appmetadata.h"

namespace utils
{
class AppSettings
{
    Q_DISABLE_COPY(AppSettings)

public:
    explicit AppSettings(const common::AppMetadata& app_metadata);
    virtual ~AppSettings() = default;

    const common::AppMetadata& getAppMetadata() const;
    quint16                    getPort() const;
    const QString&             getLoggingRules() const;
    const QString&             getSunshineAppsFilepath() const;
    bool                       getPreferHibernation() const;
    QSsl::SslProtocol          getSslProtocol() const;
    bool                       getCloseSteamBeforeSleep() const;
    const QString&             getSteamExecutablePath() const;
    const QString&             getMacAddressOverride() const;
    const QRegularExpression&  getEnvCaptureRegex() const;

private:
    bool parseSettingsFile(const QString& filepath);
    void saveDefaultFile(const QString& filepath) const;

    const common::AppMetadata& m_app_metadata;
    quint16                    m_port;
    QString                    m_logging_rules;
    QString                    m_sunshine_apps_filepath;
    bool                       m_prefer_hibernation;
    QSsl::SslProtocol          m_ssl_protocol;
    bool                       m_close_steam_before_sleep;
    QString                    m_steam_exec_override;
    QString                    m_mac_address_override;
    QRegularExpression         m_env_capture_regex;
};
}  // namespace utils
