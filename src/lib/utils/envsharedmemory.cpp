// header include
#include "utils/envsharedmemory.h"

// system/Qt includes
#include <QCoreApplication>
#include <QDataStream>
#include <QDebug>
#include <QMutexLocker>
#include <QTimer>
#include <QProcessEnvironment>
#include <cstring>

// local includes
#include "shared/loggingcategories.h"

namespace utils
{

EnvSharedMemory::EnvSharedMemory(QObject* parent)
    : QObject(parent)
    , m_sharedMemory(getSharedMemoryKey())
{
}

EnvSharedMemory::~EnvSharedMemory()
{
    // Detach from shared memory if attached
    if (m_sharedMemory.isAttached())
    {
        m_sharedMemory.detach();
    }
}

QString EnvSharedMemory::getSharedMemoryKey()
{
    return QString::fromLatin1(SHARED_MEMORY_KEY);
}

bool EnvSharedMemory::captureAndStoreEnvironment(const QStringList& prefixes)
{
    QMutexLocker locker(&m_mutex);
    
    if (prefixes.isEmpty())
    {
        qCWarning(lc::utils) << "No prefixes provided for environment variable capture";
        return false;
    }
    
    QMap<QString, QString> envVars;
    
    // Capture environment variables with specified prefixes
    const auto systemEnv = QProcessEnvironment::systemEnvironment();
    for (const QString& key : systemEnv.keys())
    {
        for (const QString& prefix : prefixes)
        {
            if (key.startsWith(prefix, Qt::CaseInsensitive))
            {
                envVars[key] = systemEnv.value(key);
                qCDebug(lc::utils) << "Captured environment variable:" << key << "=" << systemEnv.value(key);
                break; // Found matching prefix, no need to check others
            }
        }
    }
    
    qCInfo(lc::utils) << "Captured" << envVars.size() << "environment variables for prefixes:" << prefixes;
    
    if (envVars.isEmpty())
    {
        qCInfo(lc::utils) << "No environment variables found with specified prefixes - this is normal if none are set";
        // Still try to clear any existing shared memory
        clearEnvironment();
        return true;
    }
    
    return serializeAndStore(envVars);
}

QMap<QString, QString> EnvSharedMemory::retrieveEnvironment()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_sharedMemory.isAttached() && !m_sharedMemory.attach(QSharedMemory::ReadOnly))
    {
        qCDebug(lc::utils) << "No shared memory available for environment variables - this is normal if Stream is not running";
        return {};
    }
    
    if (m_sharedMemory.size() < static_cast<int>(sizeof(EnvData)))
    {
        qCWarning(lc::utils) << "Shared memory size too small:" << m_sharedMemory.size();
        return {};
    }
    
    // Lock shared memory for reading
    if (!m_sharedMemory.lock())
    {
        qCWarning(lc::utils) << "Failed to lock shared memory for reading";
        return {};
    }
    
    const auto* envData = static_cast<const EnvData*>(m_sharedMemory.constData());
    
    if (envData->version != CURRENT_VERSION)
    {
        qCWarning(lc::utils) << "Version mismatch in shared memory. Expected:" << CURRENT_VERSION 
                             << "Got:" << envData->version;
        m_sharedMemory.unlock();
        return {};
    }
    
    if (envData->size > MAX_DATA_SIZE)
    {
        qCWarning(lc::utils) << "Environment data size too large:" << envData->size;
        m_sharedMemory.unlock();
        return {};
    }
    
    // Read the serialized data
    const char* dataPtr = static_cast<const char*>(m_sharedMemory.constData()) + sizeof(EnvData);
    QByteArray serializedData(dataPtr, envData->size);
    
    // Verify checksum
    quint32 calculatedChecksum = calculateChecksum(serializedData);
    if (calculatedChecksum != envData->checksum)
    {
        qCWarning(lc::utils) << "Checksum mismatch in shared memory data";
        m_sharedMemory.unlock();
        return {};
    }
    
    m_sharedMemory.unlock();
    
    auto result = deserializeData(serializedData);
    if (!result.isEmpty())
    {
        qCInfo(lc::utils) << "Retrieved" << result.size() << "environment variables from shared memory";
    }
    
    return result;
}

bool EnvSharedMemory::hasValidData()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_sharedMemory.isAttached() && !m_sharedMemory.attach(QSharedMemory::ReadOnly))
    {
        return false;
    }
    
    if (m_sharedMemory.size() < static_cast<int>(sizeof(EnvData)))
    {
        return false;
    }
    
    m_sharedMemory.lock();
    const auto* envData = static_cast<const EnvData*>(m_sharedMemory.constData());
    bool valid = (envData->version == CURRENT_VERSION && envData->size <= MAX_DATA_SIZE);
    m_sharedMemory.unlock();
    
    return valid;
}

bool EnvSharedMemory::clearEnvironment()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_sharedMemory.isAttached())
    {
        m_sharedMemory.detach();
    }
    
    qCInfo(lc::utils) << "Cleared environment variables from shared memory";
    return true;
}

bool EnvSharedMemory::serializeAndStore(const QMap<QString, QString>& envVars)
{
    // Serialize the environment variables
    QByteArray serializedData;
    QDataStream stream(&serializedData, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);
    
    stream << static_cast<quint32>(envVars.size());
    for (auto it = envVars.constBegin(); it != envVars.constEnd(); ++it)
    {
        stream << it.key() << it.value();
    }
    
    if (serializedData.size() > static_cast<int>(MAX_DATA_SIZE))
    {
        qCWarning(lc::utils) << "Environment variables data too large:" << serializedData.size() 
                             << "bytes (max:" << MAX_DATA_SIZE << ")";
        return false;
    }
    
    // Calculate total size needed
    int totalSize = sizeof(EnvData) + serializedData.size();
    
    // Create or attach to shared memory
    if (m_sharedMemory.isAttached())
    {
        m_sharedMemory.detach();
    }
    
    if (!m_sharedMemory.create(totalSize))
    {
        if (m_sharedMemory.error() == QSharedMemory::AlreadyExists)
        {
            if (!m_sharedMemory.attach())
            {
                qCWarning(lc::utils) << "Failed to attach to existing shared memory:" << m_sharedMemory.errorString();
                return false;
            }
            
            // Detach and recreate with correct size
            m_sharedMemory.detach();
            if (!m_sharedMemory.create(totalSize))
            {
                qCWarning(lc::utils) << "Failed to recreate shared memory:" << m_sharedMemory.errorString();
                return false;
            }
        }
        else
        {
            qCWarning(lc::utils) << "Failed to create shared memory:" << m_sharedMemory.errorString();
            return false;
        }
    }
    
    // Lock and write data
    m_sharedMemory.lock();
    
    auto* envData = static_cast<EnvData*>(m_sharedMemory.data());
    envData->version = CURRENT_VERSION;
    envData->size = serializedData.size();
    envData->checksum = calculateChecksum(serializedData);
    
    // Copy serialized data after the header
    char* dataPtr = static_cast<char*>(m_sharedMemory.data()) + sizeof(EnvData);
    std::memcpy(dataPtr, serializedData.constData(), serializedData.size());
    
    m_sharedMemory.unlock();
    
    qCInfo(lc::utils) << "Stored" << envVars.size() << "environment variables in shared memory (" 
                      << serializedData.size() << "bytes)";
    
    return true;
}

QMap<QString, QString> EnvSharedMemory::deserializeData(const QByteArray& data)
{
    QMap<QString, QString> envVars;
    QDataStream stream(data);
    stream.setVersion(QDataStream::Qt_6_0);
    
    quint32 count;
    stream >> count;
    
    if (stream.status() != QDataStream::Ok)
    {
        qCWarning(lc::utils) << "Failed to read environment variable count from stream";
        return {};
    }
    
    if (count > 1000) // Sanity check to prevent excessive memory usage
    {
        qCWarning(lc::utils) << "Environment variable count too large:" << count;
        return {};
    }
    
    for (quint32 i = 0; i < count && !stream.atEnd(); ++i)
    {
        QString key, value;
        stream >> key >> value;
        
        if (stream.status() != QDataStream::Ok)
        {
            qCWarning(lc::utils) << "Failed to read environment variable" << i << "from stream";
            break;
        }
        
        if (!key.isEmpty()) // Skip empty keys
        {
            envVars[key] = value;
        }
    }
    
    return envVars;
}

quint32 EnvSharedMemory::calculateChecksum(const QByteArray& data)
{
    // Simple checksum calculation
    quint32 checksum = 0;
    for (int i = 0; i < data.size(); ++i)
    {
        checksum += static_cast<quint8>(data[i]);
        checksum = (checksum << 1) | (checksum >> 31); // Rotate left
    }
    return checksum;
}

} // namespace utils

#include "envsharedmemory.moc"
