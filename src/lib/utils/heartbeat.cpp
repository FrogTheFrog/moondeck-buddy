// header file include
#include "utils/heartbeat.h"

// system/Qt includes
#include <QCryptographicHash>
#include <QDateTime>

//---------------------------------------------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------------------------------------------

const int  TIME_INDEX{0};
const int  TERMINATE_INDEX{1};
const uint HEARTBEAT_INTERVAL{500};
const uint HEARTBEAT_TIMEOUT{2000};

//---------------------------------------------------------------------------------------------------------------------

class HeartbeatAccessor final
{
    Q_DISABLE_COPY(HeartbeatAccessor)

public:
    explicit HeartbeatAccessor(QSharedMemory& shared_mem);
    ~HeartbeatAccessor();

    void      setTime(const QDateTime& time);
    QDateTime getTime() const;

    void setShouldTerminate(bool terminate);
    bool getShouldTerminate() const;

private:
    QSharedMemory& m_shared_mem;
};

//---------------------------------------------------------------------------------------------------------------------

HeartbeatAccessor::HeartbeatAccessor(QSharedMemory& shared_mem)
    : m_shared_mem{shared_mem}
{
    if (!m_shared_mem.lock())
    {
        qFatal("Failed to lock shared memory %s", qUtf8Printable(m_shared_mem.key()));
    }
}

//---------------------------------------------------------------------------------------------------------------------

HeartbeatAccessor::~HeartbeatAccessor()
{
    if (!m_shared_mem.unlock())
    {
        qFatal("Failed to unlock shared memory %s", qUtf8Printable(m_shared_mem.key()));
    }
}

//---------------------------------------------------------------------------------------------------------------------

void HeartbeatAccessor::setTime(const QDateTime& time)
{
    const qint64 time_ms{time.toMSecsSinceEpoch()};
    // NOLINTNEXTLINE(*-reinterpret-cast)
    qint64* mem_ptr{reinterpret_cast<qint64*>(m_shared_mem.data())};
    // NOLINTNEXTLINE(*-pointer-arithmetic)
    mem_ptr[TIME_INDEX] = time_ms;
}

//---------------------------------------------------------------------------------------------------------------------

QDateTime HeartbeatAccessor::getTime() const
{
    // NOLINTNEXTLINE(*-reinterpret-cast)
    const qint64* mem_ptr{reinterpret_cast<qint64*>(m_shared_mem.data())};
    // NOLINTNEXTLINE(*-pointer-arithmetic)
    return QDateTime::fromMSecsSinceEpoch(mem_ptr[TIME_INDEX], Qt::UTC);
}

//---------------------------------------------------------------------------------------------------------------------

void HeartbeatAccessor::setShouldTerminate(bool terminate)
{
    // NOLINTNEXTLINE(*-reinterpret-cast)
    qint64* mem_ptr{reinterpret_cast<qint64*>(m_shared_mem.data())};
    // NOLINTNEXTLINE(*-pointer-arithmetic)
    mem_ptr[TERMINATE_INDEX] = static_cast<qint64>(terminate);
}

//---------------------------------------------------------------------------------------------------------------------

bool HeartbeatAccessor::getShouldTerminate() const
{
    // NOLINTNEXTLINE(*-reinterpret-cast)
    const qint64* mem_ptr{reinterpret_cast<qint64*>(m_shared_mem.data())};
    // NOLINTNEXTLINE(*-pointer-arithmetic)
    return static_cast<bool>(mem_ptr[TERMINATE_INDEX]);
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace utils
{
Heartbeat::Heartbeat(const QString& key)
    : m_shared_mem(generateKeyHash(key, "_heartbeat_key"))
{
    m_timer.setInterval(HEARTBEAT_INTERVAL);
    m_timer.setSingleShot(true);

    if (m_shared_mem.create(sizeof(qint64) * 2))
    {
        HeartbeatAccessor memory{m_shared_mem};
        memory.setShouldTerminate(false);
        memory.setTime(QDateTime::currentDateTimeUtc().addDays(-1));
        return;
    }

    if (m_shared_mem.error() != QSharedMemory::AlreadyExists)
    {
        qFatal("Failed to create shared memory for %s (%s). Reason: %s", qUtf8Printable(key),
               qUtf8Printable(m_shared_mem.key()), qUtf8Printable(m_shared_mem.errorString()));
    }

    if (!m_shared_mem.attach())
    {
        qFatal("Failed to attach to shared memory %s (%s). Reason: %s", qUtf8Printable(key),
               qUtf8Printable(m_shared_mem.key()), qUtf8Printable(m_shared_mem.errorString()));
    }
}

//---------------------------------------------------------------------------------------------------------------------

void Heartbeat::startBeating()
{
    if (m_is_listening)
    {
        qFatal("You cannot start heartbeating if you're a listener!");
    }

    if (!m_is_beating)
    {
        m_is_beating = true;
        connect(&m_timer, &QTimer::timeout, this, [this]() { slotBeating(false); });
        slotBeating(true);
    }
}

//---------------------------------------------------------------------------------------------------------------------

void Heartbeat::startListening()
{
    if (m_is_beating)
    {
        qFatal("You cannot start listening if you're the heartbeat!");
    }

    if (!m_is_listening)
    {
        m_is_listening = true;
        connect(&m_timer, &QTimer::timeout, this, &Heartbeat::slotListening);
        slotListening();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void Heartbeat::terminate()
{
    HeartbeatAccessor memory{m_shared_mem};
    memory.setShouldTerminate(true);
}

//---------------------------------------------------------------------------------------------------------------------

bool Heartbeat::isAlive() const
{
    return m_is_beating || m_is_alive;
}

//---------------------------------------------------------------------------------------------------------------------

void Heartbeat::slotBeating(bool fresh_start)
{
    m_timer.stop();

    HeartbeatAccessor memory{m_shared_mem};
    if (!fresh_start && memory.getShouldTerminate())
    {
        emit signalShouldTerminate();
        return;
    }

    memory.setShouldTerminate(false);
    memory.setTime(QDateTime::currentDateTimeUtc());

    m_timer.start();
}

//---------------------------------------------------------------------------------------------------------------------

void Heartbeat::slotListening()
{
    m_timer.stop();

    const HeartbeatAccessor memory{m_shared_mem};
    const QDateTime         last_beat{memory.getTime()};
    const bool              is_alive{last_beat.msecsTo(QDateTime::currentDateTimeUtc()) <= HEARTBEAT_TIMEOUT};

    if (is_alive != m_is_alive)
    {
        m_is_alive = is_alive;
        emit signalStateChanged();
    }

    m_timer.start();
}
}  // namespace utils
