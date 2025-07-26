#pragma once

// system/Qt includes
#include <QIODevice>
#include <QSharedMemory>

namespace utils
{
//! Writes data stream into shared memory.
class ShmSerializer final
{
    Q_DISABLE_COPY(ShmSerializer)

public:
    explicit ShmSerializer(const QString& key);
    ~ShmSerializer() = default;

    template<typename T>
    bool write(const T& data)
    {
        QByteArray bytes;
        {
            QDataStream stream(&bytes, QIODevice::WriteOnly);
            stream << data;
        }
        return write(bytes);
    }

private:
    bool write(const QByteArray& data);

    QSharedMemory m_shared_mem;
};

//! Reads data stream from shared memory.
class ShmDeserializer final
{
    Q_DISABLE_COPY(ShmDeserializer)

public:
    explicit ShmDeserializer(const QString& key);
    ~ShmDeserializer() = default;

    template<typename T>
    std::optional<T> read()
    {
        if (auto bytes = read(); bytes)
        {
            QDataStream stream(&*bytes, QIODevice::ReadOnly);

            T data{};
            stream >> data;
            return data;
        }

        return std::nullopt;
    }

private:
    std::optional<QByteArray> read() const;

    QString m_key;
};

}  // namespace utils
