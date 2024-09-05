#pragma once

// system/Qt includes
#include <QObject>

//---------------------------------------------------------------------------------------------------------------------

namespace enums
{
Q_NAMESPACE

//---------------------------------------------------------------------------------------------------------------------

enum class PcState
{
    Normal,
    Restarting,
    ShuttingDown,
    Suspending
};
Q_ENUM_NS(PcState)

//---------------------------------------------------------------------------------------------------------------------

enum class StreamState
{
    NotStreaming,
    Streaming,
    StreamEnding
};
Q_ENUM_NS(StreamState)
}  // namespace enums