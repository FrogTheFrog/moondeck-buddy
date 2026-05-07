#pragma once

// system/Qt includes
#include <QMetaEnum>
#include <QObject>

namespace enums
{
Q_NAMESPACE

enum class SteamUiMode
{
    Unknown,
    Desktop,
    BigPicture
};
Q_ENUM_NS(SteamUiMode)

enum class AppState
{
    Stopped,
    Running,
    Updating
};
Q_ENUM_NS(AppState)

enum class PcState
{
    Normal,
    Restarting,
    ShuttingDown,
    Suspending,
    Transient
};
Q_ENUM_NS(PcState)

enum class StreamState
{
    NotStreaming,
    Streaming,
    StreamEnding
};
Q_ENUM_NS(StreamState)

template<class T>
const std::vector<T>& qEnumValues()
{
    static const std::vector<T> all_values{[]()
                                           {
                                               const auto     enum_size{QMetaEnum::fromType<T>().keyCount()};
                                               std::vector<T> values;

                                               for (int i = 0; i < enum_size; ++i)
                                               {
                                                   const auto value{static_cast<T>(QMetaEnum::fromType<T>().value(i))};
                                                   values.emplace_back(value);
                                               }

                                               return values;
                                           }()};

    return all_values;
}

template<class T>
QString qEnumToString(const T value)
{
    return QMetaEnum::fromType<T>().valueToKey(static_cast<int>(value));
}

template<class T>
std::optional<T> qEnumFromInt(const int value)
{
    const auto enum_size{QMetaEnum::fromType<T>().keyCount()};
    for (int i = 0; i < enum_size; ++i)
    {
        const auto enum_value{QMetaEnum::fromType<T>().value(i)};
        if (enum_value == value)
        {
            return static_cast<T>(enum_value);
        }
    }

    return std::nullopt;
}

template<class T>
std::optional<T> qEnumFromIntString(const QString& value)
{
    bool       converted{false};
    const auto number{value.toInt(&converted)};

    if (converted)
    {
        return qEnumFromInt<T>(number);
    }

    return std::nullopt;
}

template<class T>
std::optional<T> qEnumFromUInt(const std::uint32_t value)
{
    return qEnumFromInt<T>(static_cast<int>(value));
}
}  // namespace enums