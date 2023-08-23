// header file include
#include "appsettings.h"

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

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const quint16 DEFAULT_PORT{59999};

//---------------------------------------------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------------------------------------------

namespace utils
{
AppSettings::AppSettings(const QString& filepath)
    : m_port{DEFAULT_PORT}
    , m_prefer_hibernation{false}
    , m_ssl_protocol{QSsl::SecureProtocols}
    , m_force_big_picture{true}
    , m_nvidia_reset_mouse_acceleration_after_stream_end_hack{false}
{
    if (!parseSettingsFile(filepath))
    {
        qCInfo(lc::utils) << "Saving default settings to" << filepath;
        saveDefaultFile(filepath);
        if (!parseSettingsFile(filepath))
        {
            qFatal("Failed to parse \"%s\"!", qUtf8Printable(filepath));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

quint16 AppSettings::getPort() const
{
    return m_port;
}

//---------------------------------------------------------------------------------------------------------------------

const QString& AppSettings::getLoggingRules() const
{
    return m_logging_rules;
}

//---------------------------------------------------------------------------------------------------------------------

const std::set<QString>& AppSettings::getHandledDisplays() const
{
    return m_handled_displays;
}

//---------------------------------------------------------------------------------------------------------------------

const QString& AppSettings::getSunshineAppsFilepath() const
{
    return m_sunshine_apps_filepath;
}

//---------------------------------------------------------------------------------------------------------------------

bool AppSettings::getPreferHibernation() const
{
    return m_prefer_hibernation;
}

//---------------------------------------------------------------------------------------------------------------------

QSsl::SslProtocol AppSettings::getSslProtocol() const
{
    return m_ssl_protocol;
}

//---------------------------------------------------------------------------------------------------------------------

bool AppSettings::getForceBigPicture() const
{
    return m_force_big_picture;
}

//---------------------------------------------------------------------------------------------------------------------

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
            const auto obj_v                    = json_data.object();
            const auto port_v                   = obj_v.value(QLatin1String("port"));
            const auto logging_rules_v          = obj_v.value(QLatin1String("logging_rules"));
            const auto handled_displays_v       = obj_v.value(QLatin1String("handled_displays"));
            const auto sunshine_apps_filepath_v = obj_v.value(QLatin1String("sunshine_apps_filepath"));
            const auto prefer_hibernation_v     = obj_v.value(QLatin1String("prefer_hibernation"));
            const auto ssl_protocol_v           = obj_v.value(QLatin1String("ssl_protocol"));
            const auto force_big_picture_v      = obj_v.value(QLatin1String("force_big_picture"));

            // TODO: remove
            const auto nvidia_reset_mouse_acceleration_after_stream_end_hack_v =
                obj_v.value(QLatin1String("nvidia_reset_mouse_acceleration_after_stream_end_hack"));

            // TODO: dec. once removed
            constexpr int current_entries{8};
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

            if (handled_displays_v.isArray())
            {
                m_handled_displays.clear();

                bool             some_entries_were_skipped{false};
                const QJsonArray entries = handled_displays_v.toArray();
                for (const auto& entry : entries)
                {
                    const QString entry_string{entry.isString() ? entry.toString() : QString{}};
                    if (entry_string.isEmpty() || m_logging_rules.contains(entry_string))
                    {
                        some_entries_were_skipped = true;
                        continue;
                    }

                    m_handled_displays.insert(entry_string);
                }

                if (!some_entries_were_skipped)
                {
                    valid_entries++;
                }
            }

            if (sunshine_apps_filepath_v.isString())
            {
                m_sunshine_apps_filepath = sunshine_apps_filepath_v.toString();
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

            if (force_big_picture_v.isBool())
            {
                m_force_big_picture = force_big_picture_v.toBool();
                valid_entries++;
            }

            if (nvidia_reset_mouse_acceleration_after_stream_end_hack_v.isBool())
            {
                m_nvidia_reset_mouse_acceleration_after_stream_end_hack =
                    nvidia_reset_mouse_acceleration_after_stream_end_hack_v.toBool();
                valid_entries++;
            }

            return valid_entries == current_entries;
        }
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------

void AppSettings::saveDefaultFile(const QString& filepath) const
{
    QJsonObject obj;

    obj["port"]          = m_port;
    obj["logging_rules"] = m_logging_rules;
    obj["handled_displays"] =
        QJsonArray::fromStringList({std::cbegin(m_handled_displays), std::cend(m_handled_displays)});
    obj["sunshine_apps_filepath"] = m_sunshine_apps_filepath;
    obj["prefer_hibernation"]     = m_prefer_hibernation;
    obj["ssl_protocol"]           = QStringLiteral("SecureProtocols");
    obj["force_big_picture"]      = m_force_big_picture;
    obj["nvidia_reset_mouse_acceleration_after_stream_end_hack"] =
        m_nvidia_reset_mouse_acceleration_after_stream_end_hack;

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
