#pragma once

// system/Qt includes
#include <QObject>
#include <QSharedMemory>
#include <QStringList>
#include <QMap>
#include <QMutex>
#include <QProcessEnvironment>

namespace utils
{

/**
 * @brief Class for transferring environment variables between Stream and Buddy applications
 * using shared memory. Captures APOLLO* and SUNSHINE* environment variables from Stream
 * and makes them available to Buddy for game launching.
 */
class EnvSharedMemory : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(EnvSharedMemory)

public:
    explicit EnvSharedMemory(QObject* parent = nullptr);
    ~EnvSharedMemory() override;

    /**
     * @brief Captures environment variables starting with specified prefixes and stores them in shared memory
     * @param prefixes List of prefixes to capture (e.g., {"APOLLO", "SUNSHINE"})
     * @return true if successful, false otherwise
     */
    bool captureAndStoreEnvironment(const QStringList& prefixes = {"APOLLO", "SUNSHINE"});

    /**
     * @brief Retrieves environment variables from shared memory
     * @return Map of environment variable names to values, empty if failed
     */
    QMap<QString, QString> retrieveEnvironment();

    /**
     * @brief Checks if shared memory contains valid environment data
     * @return true if valid data exists, false otherwise
     */
    bool hasValidData();

    /**
     * @brief Clears the shared memory segment
     * @return true if successful, false otherwise
     */
    bool clearEnvironment();

    /**
     * @brief Gets the name of the shared memory segment
     * @return The shared memory key name
     */
    static QString getSharedMemoryKey();

private:
    struct EnvData
    {
        quint32 version;        // Data format version for compatibility
        quint32 size;          // Size of serialized environment data
        quint32 checksum;      // Simple checksum for data validation
        // Variable length data follows...
    };

    static constexpr quint32 CURRENT_VERSION = 1;
    static constexpr quint32 MAX_DATA_SIZE = 64 * 1024; // 64KB max for env vars
    static constexpr char SHARED_MEMORY_KEY[] = "MoonDeckBuddy_EnvVars";

    bool serializeAndStore(const QMap<QString, QString>& envVars);
    QMap<QString, QString> deserializeData(const QByteArray& data);
    quint32 calculateChecksum(const QByteArray& data);
    
    QSharedMemory m_sharedMemory;
    mutable QMutex m_mutex;
};

} // namespace utils
