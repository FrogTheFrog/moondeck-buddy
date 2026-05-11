// header file include
#include "server/clientids.h"

// system/Qt includes
#include <QDir>
#include <QFile>
#include <QFileInfo>

// local includes
#include "common/loggingcategories.h"
#include "json/json.h"

namespace server
{
ClientIds::ClientIds(QString filepath)
    : m_filepath{std::move(filepath)}
{
}

void ClientIds::load()
{
    // Clear the ids regardless of whether the file exists or not
    m_ids.clear();

    if (QFile ids_file{m_filepath}; ids_file.exists())
    {
        if (!ids_file.open(QFile::ReadOnly))
        {
            qFatal("File exists, but could not be opened: \"%s\"", qUtf8Printable(m_filepath));
        }

        if (const auto data{ids_file.readAll()}; !data.isEmpty())
        {
            const auto parsed_ids{json::fromJson<std::set<QString>>(data)};
            if (!parsed_ids)
            {
                qFatal("Failed to decode JSON data from \"%s\"! Reason:\n%s", qUtf8Printable(m_filepath),
                       qUtf8Printable(parsed_ids.error()));
            }

            m_ids = parsed_ids.value();
            m_ids.erase(QString{});
        }
    }
}

void ClientIds::save() const
{
    QFile file{m_filepath};
    if (!file.exists())
    {
        const QFileInfo info(m_filepath);
        if (const QDir dir; !dir.mkpath(info.absolutePath()))
        {
            qFatal("Failed at mkpath: \"%s\".", qUtf8Printable(m_filepath));
        }
    }

    if (!file.open(QFile::WriteOnly))
    {
        qFatal("File could not be opened for writing: \"%s\".", qUtf8Printable(m_filepath));
    }

    const auto serialized_ids{json::toJson<{.m_indentation = 4}>(m_ids)};
    if (!serialized_ids)
    {
        qFatal("Failed to encode JSON data! Reason:\n%s", qUtf8Printable(serialized_ids.error()));
    }

    file.write(serialized_ids.value().toUtf8());
    qCInfo(lc::server) << "Finished saving:" << m_filepath;
}

bool ClientIds::containsId(const QString& client_id) const
{
    return m_ids.contains(client_id);
}

void ClientIds::addId(const QString& client_id)
{
    m_ids.emplace(client_id);
}

void ClientIds::removeId(const QString& client_id)
{
    m_ids.erase(client_id);
}
}  // namespace server