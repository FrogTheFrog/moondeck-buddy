// header file include
#include "appsettings.h"

// system/Qt includes
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <limits>

//---------------------------------------------------------------------------------------------------------------------

namespace utils
{
AppSettings::AppSettings(const QString& filename)
{
    if (!parseSettingsFile(filename))
    {
        qInfo("Saving default settings to \"%s\".", qUtf8Printable(filename));
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

bool AppSettings::isVerbose() const
{
    return m_verbose;
}

//---------------------------------------------------------------------------------------------------------------------

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
            const auto obj_v     = json_data.object();
            const auto port_v    = obj_v.value(QLatin1String("port"));
            const auto verbose_v = obj_v.value(QLatin1String("verbose"));

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

            if (verbose_v.isBool())
            {
                m_verbose = verbose_v.toBool();
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

    obj["port"]    = m_port;
    obj["verbose"] = m_verbose;

    QFile file{filename};
    if (!file.open(QFile::WriteOnly))
    {
        qFatal("File could not be opened for writting: \"%s\".", qUtf8Printable(filename));
    }

    const QJsonDocument file_data{obj};
    file.write(file_data.toJson(QJsonDocument::Indented));
}
}  // namespace utils
