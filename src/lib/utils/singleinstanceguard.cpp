// header file include
#include "utils/singleinstanceguard.h"

#if defined(QT_OS_WINDOWS)
    // A SEPARATE WINDOWS INCLUDE BECAUSE OF THE SMELL!
    #include <windows.h>
#endif

// system/Qt includes
#include <QCryptographicHash>

namespace
{
QString generateKeyHash(const QString& key, const QString& salt)
{
    QByteArray data;

    data.append(key.toUtf8());
    data.append(salt.toUtf8());
    data = QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();

    return data;
}
}  // namespace

namespace utils
{
SingleInstanceGuard::SingleInstanceGuard(const QString& key)
    : m_mem_lock_key(generateKeyHash(key, "_mem_lock_key"))
    , m_shared_mem_key(generateKeyHash(key, "_shared_mem_key"))
    , m_mem_lock(m_mem_lock_key, QSystemSemaphore::Create)
    , m_shared_mem(m_shared_mem_key)
{
    auto cleanup = qScopeGuard([this]() { m_mem_lock.release(); });
    m_mem_lock.acquire();
    {
        QSharedMemory fix(m_shared_mem_key);
        fix.attach();
    }

#if defined(QT_OS_WINDOWS)
    // OS will clean up this mutex (used by installer)
    CreateMutexA(nullptr, FALSE, key.toLatin1());
#endif
}

SingleInstanceGuard::~SingleInstanceGuard()
{
    release();
}

bool SingleInstanceGuard::isAnotherRunning()
{
    if (m_shared_mem.isAttached())
    {
        return false;
    }

    auto cleanup = qScopeGuard([this]() { m_mem_lock.release(); });
    m_mem_lock.acquire();

    const bool is_running = m_shared_mem.attach();
    if (is_running)
    {
        m_shared_mem.detach();
    }

    return is_running;
}

bool SingleInstanceGuard::tryToRun()
{
    if (isAnotherRunning())
    {
        return false;
    }

    const bool result = [this]()
    {
        auto cleanup = qScopeGuard([this]() { m_mem_lock.release(); });
        m_mem_lock.acquire();
        return m_shared_mem.create(sizeof(quint64));
    }();

    if (!result)
    {
        release();
        return false;
    }

    return true;
}

void SingleInstanceGuard::release()
{
    auto cleanup = qScopeGuard([this]() { m_mem_lock.release(); });
    m_mem_lock.acquire();

    if (m_shared_mem.isAttached())
    {
        m_shared_mem.detach();
    }
}
}  // namespace utils
