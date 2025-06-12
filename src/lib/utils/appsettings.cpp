// header file include
#include "utils/appsettings.h"

// system/Qt includes
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <limits>
#include <optional>
#include <unordered_map>

// local includes
#include "shared/loggingcategories.h"

namespace
{
const quint16 DEFAULT_PORT{59999};

std::optional<QSsl::SslProtocol> protocolFromString(const QString& value)
{
    static const std::unordered_map<QString, QSsl::SslProtocol> allowed_values{
        {QStringLiteral("SecureProtocols"), QSsl::SecureProtocols},  //
        {QStringLiteral("TlsV1_2"), QSsl::TlsV1_2},                  //
        {QStringLiteral("TlsV1_2OrLater"), QSsl::TlsV1_2OrLater},    //
        {QStringLiteral("TlsV1_3"), QSsl::TlsV1_3},                  //
        {QStringLiteral("TlsV1_3OrLater"), QSsl::TlsV1_3OrLater}     //
    };

    auto value_it{allowed_values.find(value)};
    if (value_it == std::end(allowed_values))
    {
        return std::nullopt;
    }

    // Delayed logging
    QTimer::singleShot(0, [value, enum_value = value_it->second]()
                       { qCDebug(lc::utils) << "Mapped" << value << "to" << enum_value; });
    return value_it->second;
}
}  // namespace

namespace utils
{
AppSettings::AppSettings(const shared::AppMetadata& app_metadata)
    : m_app_metadata{app_metadata}
    , m_port{DEFAULT_PORT}
    , m_prefer_hibernation{false}
    , m_ssl_protocol{QSsl::SecureProtocols}
    , m_close_steam_before_sleep{true}
{
    auto settings_path{m_app_metadata.getSettingsPath()};
    if (!parseSettingsFile(settings_path))
    {
        qCInfo(lc::utils) << "Saving default settings to" << settings_path;
        saveDefaultFile(settings_path);
        if (!parseSettingsFile(settings_path))
        {
            qFatal("Failed to parse \"%s\"!", qUtf8Printable(settings_path));
        }
    }
}

const shared::AppMetadata& AppSettings::getAppMetadata() const
{
    return m_app_metadata;
}

quint16 AppSettings::getPort() const
{
    return m_port;
}

const QString& AppSettings::getLoggingRules() const
{
    return m_logging_rules;
}

bool AppSettings::getPreferHibernation() const
{
    return m_prefer_hibernation;
}

QSsl::SslProtocol AppSettings::getSslProtocol() const
{
    return m_ssl_protocol;
}

bool AppSettings::getCloseSteamBeforeSleep() const
{
    return m_close_steam_before_sleep;
}

QString AppSettings::getSteamExecutablePath() const
{
    const auto& exec_path{m_steam_exec_override.isEmpty() ? m_app_metadata.getDefaultSteamExecutable()
                                                          : m_steam_exec_override};
    if (QFileInfo::exists(exec_path))
    {
        return exec_path;
    }

    return QString{};
}

const QString& AppSettings::getMacAddressOverride() const
{
    return m_mac_address_override;
}

// NOLINTNEXTLINE(*-function-cognitive-complexity)
bool AppSettings::parseSettingsFile(const QString& filepath)
{
    QFile file{filepath};
    if (file.exists())
    {
        if (!file.open(QFile::ReadOnly))
        {
            qFatal("File exists, but could not be opened: \"%s\"", qUtf8Printable(filepath));
        }

        const QByteArray data = file.readAll();

        QJsonParseError     parser_error;
        const QJsonDocument json_data{QJsonDocument::fromJson(data, &parser_error)};
        if (json_data.isNull())
        {
            qFatal("Failed to decode JSON data! Reason: %s. Read data: %s", qUtf8Printable(parser_error.errorString()),
                   qUtf8Printable(data));
        }
        else if (!json_data.isEmpty())
        {
            const auto obj_v                      = json_data.object();
            const auto port_v                     = obj_v.value(QLatin1String("port"));
            const auto logging_rules_v            = obj_v.value(QLatin1String("logging_rules"));
            const auto prefer_hibernation_v       = obj_v.value(QLatin1String("prefer_hibernation"));
            const auto ssl_protocol_v             = obj_v.value(QLatin1String("ssl_protocol"));
            const auto close_steam_before_sleep_v = obj_v.value(QLatin1String("close_steam_before_sleep"));
            const auto mac_address_override_v     = obj_v.value(QLatin1String("mac_address_override"));
            const auto steam_exec_override_v      = obj_v.value(QLatin1String("steam_exec_override"));

            constexpr int current_entries{7};
            int           valid_entries{0};

            if (port_v.isDouble())
            {
                const auto port = port_v.toInt(-1);
                const int  port_min{0};
                const int  port_max{std::numeric_limits<quint16>::max()};

                if (port < port_min || port_max < port)
                {
                    qFatal("Port value (%d) is out of range!", port);
                }

                m_port = static_cast<quint16>(port);
                valid_entries++;
            }

            if (logging_rules_v.isString())
            {
                m_logging_rules = logging_rules_v.toString();
                valid_entries++;
            }

            if (prefer_hibernation_v.isBool())
            {
                m_prefer_hibernation = prefer_hibernation_v.toBool();
                valid_entries++;
            }

            if (ssl_protocol_v.isString())
            {
                if (auto protocol = protocolFromString(ssl_protocol_v.toString()); protocol)
                {
                    m_ssl_protocol = *protocol;
                    valid_entries++;
                }
            }

            if (close_steam_before_sleep_v.isBool())
            {
                m_close_steam_before_sleep = close_steam_before_sleep_v.toBool();
                valid_entries++;
            }

            if (mac_address_override_v.isString())
            {
                m_mac_address_override = mac_address_override_v.toString().trimmed();
                valid_entries++;
            }

            if (steam_exec_override_v.isString())
            {
                m_steam_exec_override = steam_exec_override_v.toString();
                valid_entries++;
            }

            return valid_entries == current_entries;
        }
    }

    return false;
}

void AppSettings::saveDefaultFile(const QString& filepath) const
{
    QJsonObject obj;

    obj["port"]                     = m_port;
    obj["logging_rules"]            = m_logging_rules;
    obj["prefer_hibernation"]       = m_prefer_hibernation;
    obj["ssl_protocol"]             = QStringLiteral("SecureProtocols");
    obj["close_steam_before_sleep"] = m_close_steam_before_sleep;
    obj["mac_address_override"]     = m_mac_address_override;
    obj["steam_exec_override"]      = m_steam_exec_override;

    QFile file{filepath};
    if (!file.exists())
    {
        const QFileInfo info(filepath);
        const QDir      dir;
        if (!dir.mkpath(info.absolutePath()))
        {
            qFatal("Failed at mkpath: \"%s\".", qUtf8Printable(filepath));
        }
    }

    if (!file.open(QFile::WriteOnly))
    {
        qFatal("File could not be opened for writing: \"%s\".", qUtf8Printable(filepath));
    }

    const QJsonDocument file_data{obj};
    file.write(file_data.toJson(QJsonDocument::Indented));
}
}  // namespace utils
