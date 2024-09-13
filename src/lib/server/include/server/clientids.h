#pragma once

// system/Qt includes
#include <QString>
#include <set>

namespace server
{
class ClientIds
{
    Q_DISABLE_COPY(ClientIds)

public:
    explicit ClientIds(QString filepath);
    virtual ~ClientIds() = default;

    void load();
    void save();

    bool containsId(const QString& client_id) const;
    void addId(const QString& client_id);
    void removeId(const QString& client_id);

private:
    QString           m_filepath;
    std::set<QString> m_ids;
};
}  // namespace server
