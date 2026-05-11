// header file include
#include "server/clientids.h"

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
    m_ids = json::tryPartialReadFromFile<std::set<QString>>(m_filepath);
    m_ids.erase(QString{});
}

void ClientIds::save() const
{
    json::saveToFile(m_filepath, m_ids);
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