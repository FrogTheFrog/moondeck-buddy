#pragma once

// system/Qt includes
#include <QString>
#include <QtNetwork/QSsl>
#include <set>

// local includes
#include "shared/appmetadata.h"

namespace utils
{
class AppSettings
{
    Q_DISABLE_COPY(AppSettings)

public:
    explicit AppSettings(const shared::AppMetadata& app_metadata);
    virtual ~AppSettings() = default;

    const shared::AppMetadata& getAppMetadata() const;
    quint16                    getPort() const;
    const QString&             getLoggingRules() const;
    bool                       getPreferHibernation() const;
    QSsl::SslProtocol          getSslProtocol() const;
    bool                       getCloseSteamBeforeSleep() const;
    QString                    getSteamExecutablePath() const;
    const QString&             getMacAddressOverride() const;

private:
    bool parseSettingsFile(const QString& filepath);
    void saveDefaultFile(const QString& filepath) const;

    const shared::AppMetadata& m_app_metadata;
    quint16                    m_port;
    QString                    m_logging_rules;
    bool                       m_prefer_hibernation;
    QSsl::SslProtocol          m_ssl_protocol;
    bool                       m_close_steam_before_sleep;
    QString                    m_steam_exec_override;
    QString                    m_mac_address_override;
};
}  // namespace utils
