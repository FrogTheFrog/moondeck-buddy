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
    static const std::vector<T> values{[]()
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

    return values;
}

template<class T>
QString qEnumToString(const T value)
{
    return QMetaEnum::fromType<T>().valueToKey(static_cast<int>(value));
}
}  // namespace enums