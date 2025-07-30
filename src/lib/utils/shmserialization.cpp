// header include
#include "utils/shmserialization.h"

// system/Qt includes
#include <QCryptographicHash>
#include <QProcessEnvironment>

// local includes
#include "shared/loggingcategories.h"

namespace
{
QString generateKeyHash(const QString& key)
{
    QByteArray data;

    data.append(key.toUtf8());
    data = QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();

    return data;
}

}  // namespace

namespace utils
{

ShmSerializer::ShmSerializer(const QString& key)
    : m_shared_mem(generateKeyHash(key))
{
}

bool ShmSerializer::write(const QByteArray& data)
{
    // On UNIX the shared memory segment will survive a crash, so we have to make sure to clean it up in such cases.
    const auto try_create_shared_mem = [&](auto size)
    {
        auto result{m_shared_mem.create(size)};
#if defined(Q_OS_LINUX)
        if (!result)
        {
            m_shared_mem.attach();
            m_shared_mem.detach();
            result = m_shared_mem.create(size);
        }
#endif
        return result;
    };

    if (!try_create_shared_mem(data.size()))
    {
        qCWarning(lc::utils) << "Failed to create shared memory for writing:" << m_shared_mem.errorString();
        return false;
    }

    const auto cleanup{qScopeGuard([this]() { m_shared_mem.unlock(); })};
    if (!m_shared_mem.lock())
    {
        qCWarning(lc::utils) << "Failed to lock shared memory for writing:" << m_shared_mem.errorString();
        return false;
    }

    std::memcpy(m_shared_mem.data(), data.constData(), data.size());
    return true;
}

ShmDeserializer::ShmDeserializer(const QString& key)
    : m_key{generateKeyHash(key)}
{
}

std::optional<QByteArray> ShmDeserializer::read() const
{
    QSharedMemory shared_mem{m_key};
    if (!shared_mem.attach(QSharedMemory::ReadOnly))
    {
        qCDebug(lc::utils) << "Failed to attach to shared memory for reading:" << shared_mem.errorString();
        return std::nullopt;
    }

    const auto cleanup{qScopeGuard([&shared_mem]() { shared_mem.unlock(); })};
    if (!shared_mem.lock())
    {
        qCWarning(lc::utils) << "Failed to lock shared memory for reading:" << shared_mem.errorString();
        return std::nullopt;
    }

    QByteArray data;
    data.resize(shared_mem.size());

    std::memcpy(data.data(), shared_mem.constData(), data.size());
    return data;
}

}  // namespace utils
