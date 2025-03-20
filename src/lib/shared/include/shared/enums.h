#pragma once

// system/Qt includes
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
    Suspending
};
Q_ENUM_NS(PcState)

enum class StreamState
{
    NotStreaming,
    Streaming,
    StreamEnding
};
Q_ENUM_NS(StreamState)
}  // namespace enums