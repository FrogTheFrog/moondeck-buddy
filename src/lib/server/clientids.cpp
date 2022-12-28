// header file include
#include "clientids.h"

// system/Qt includes
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace server
{
ClientIds::ClientIds(QString filename)
    : m_filename{std::move(filename)}
{
}

//---------------------------------------------------------------------------------------------------------------------

void ClientIds::load()
{
    m_ids.clear();  // Clear the ids regardless of whether the file exists or not

    QFile ids_file{m_filename};
    if (ids_file.exists())
    {
        if (!ids_file.open(QFile::ReadOnly))
        {
            qFatal("File exists, but could not be opened: \"%s\"", qUtf8Printable(m_filename));
        }

        const QByteArray data = ids_file.readAll();

        QJsonParseError     parser_error;
        const QJsonDocument json_data{QJsonDocument::fromJson(data, &parser_error)};
        if (json_data.isNull())
        {
            qFatal("Failed to decode JSON data! Reason: %s. Read data: %s", qUtf8Printable(parser_error.errorString()),
                   qUtf8Printable(data));
        }
        else if (!json_data.isEmpty())
        {
            if (!json_data.isArray())
            {
                qFatal("Client Ids file contains invalid JSON data!");
            }

            bool             some_ids_were_skipped{false};
            const QJsonArray ids = json_data.array();
            for (const auto& client_id : ids)
            {
                QString client_id_string{client_id.isString() ? client_id.toString() : QString{}};
                if (client_id_string.isEmpty())
                {
                    some_ids_were_skipped = true;
                    continue;
                }

                m_ids.emplace(client_id_string);
            }

            if (some_ids_were_skipped)
            {
                qCWarning(lc::server) << "Client Ids file contained ids that were skipped!";
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

void ClientIds::save()
{
    QJsonArray json_data;
    for (const auto& client_id : m_ids)
    {
        json_data.append(client_id);
    }

    QFile ids_file{m_filename};
    if (!ids_file.open(QFile::WriteOnly))
    {
        qFatal("File could not be opened for writting: \"%s\".", qUtf8Printable(m_filename));
    }

    const QJsonDocument file_data{json_data};
    ids_file.write(file_data.toJson(QJsonDocument::Indented));
    qCDebug(lc::server) << "Finished saving:" << m_filename;
}

//---------------------------------------------------------------------------------------------------------------------

bool ClientIds::containsId(const QString& client_id) const
{
    return m_ids.contains(client_id);
}

//---------------------------------------------------------------------------------------------------------------------

void ClientIds::addId(const QString& client_id)
{
    m_ids.emplace(client_id);
}

//---------------------------------------------------------------------------------------------------------------------

void ClientIds::removeId(const QString& client_id)
{
    m_ids.erase(client_id);
}
}  // namespace server