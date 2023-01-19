// header file include
#include "appsettings.h"

// system/Qt includes
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <limits>

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const quint16 DEFAULT_PORT{59999};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace utils
{
AppSettings::AppSettings(const QString& filepath)
    : m_port{DEFAULT_PORT}
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
            const auto obj_v           = json_data.object();
            const auto port_v          = obj_v.value(QLatin1String("port"));
            const auto logging_rules_v = obj_v.value(QLatin1String("logging_rules"));

            // TODO: remove
            const auto nvidia_reset_mouse_acceleration_after_stream_end_hack_v =
                obj_v.value(QLatin1String("nvidia_reset_mouse_acceleration_after_stream_end_hack"));

            // TODO: dec. once removed
            constexpr int current_entries{3};
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
