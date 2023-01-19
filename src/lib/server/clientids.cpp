// header file include
#include "clientids.h"

// system/Qt includes
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace server
{
ClientIds::ClientIds(QString filepath)
    : m_filepath{std::move(filepath)}
{
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-cognitive-complexity)
void ClientIds::load()
{
    m_ids.clear();  // Clear the ids regardless of whether the file exists or not

    QFile ids_file{m_filepath};
    if (ids_file.exists())
    {
        if (!ids_file.open(QFile::ReadOnly))
        {
            qFatal("File exists, but could not be opened: \"%s\"", qUtf8Printable(m_filepath));
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
                const QString client_id_string{client_id.isString() ? client_id.toString() : QString{}};
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

    QFile file{m_filepath};
    if (!file.exists())
    {
        QFileInfo info(m_filepath);
        QDir      dir;
        if (!dir.mkpath(info.absolutePath()))
        {
            qFatal("Failed at mkpath: \"%s\".", qUtf8Printable(m_filepath));
        }
    }

    if (!file.open(QFile::WriteOnly))
    {
        qFatal("File could not be opened for writing: \"%s\".", qUtf8Printable(m_filepath));
    }

    const QJsonDocument file_data{json_data};
    file.write(file_data.toJson(QJsonDocument::Indented));
    qCInfo(lc::server) << "Finished saving:" << m_filepath;
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