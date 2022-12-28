// header file include
#include "appsettings.h"

// system/Qt includes
#include <QFile>
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
AppSettings::AppSettings(const QString& filename)
    : m_port{DEFAULT_PORT}
    , m_logging_rules{}
{
    if (!parseSettingsFile(filename))
    {
        qCInfo(lc::utils) << "Saving default settings to" << filename;
        saveDefaultFile(filename);
        if (!parseSettingsFile(filename))
        {
            qFatal("Failed to parse \"%s\"!", qUtf8Printable(filename));
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
bool AppSettings::parseSettingsFile(const QString& filename)
{
    QFile file{filename};
    if (file.exists())
    {
        if (!file.open(QFile::ReadOnly))
        {
            qFatal("File exists, but could not be opened: \"%s\"", qUtf8Printable(filename));
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

            constexpr int current_entries{2};
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

            return valid_entries == current_entries;
        }
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------

void AppSettings::saveDefaultFile(const QString& filename) const
{
    QJsonObject obj;

    obj["port"]          = m_port;
    obj["logging_rules"] = m_logging_rules;

    QFile file{filename};
    if (!file.open(QFile::WriteOnly))
    {
        qFatal("File could not be opened for writting: \"%s\".", qUtf8Printable(filename));
    }

    const QJsonDocument file_data{obj};
    file.write(file_data.toJson(QJsonDocument::Indented));
}
}  // namespace utils
