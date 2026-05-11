#pragma once

// system/Qt includes
#include <QRegularExpression>

namespace common
{
struct UserSettings
{
    Q_GADGET

public:
    enum class SslProtocol
    {
        SecureProtocols,
        TlsV1_2,
        TlsV1_2OrLater,
        TlsV1_3,
        TlsV1_3OrLater
    };
    Q_ENUM(SslProtocol)

    static UserSettings loadAndValidate(const QString& filepath);

    quint16            m_port{59999};
    QString            m_logging_rules;
    QString            m_sunshine_apps_filepath;
    bool               m_prefer_hibernation{false};
    SslProtocol        m_ssl_protocol{SslProtocol::SecureProtocols};
    bool               m_close_steam_before_sleep{true};
    QString            m_steam_exec_override;
    QString            m_mac_address_override;
    QRegularExpression m_env_capture_regex{"^(?:SUNSHINE|APOLLO).*"};
};
}  // namespace common
