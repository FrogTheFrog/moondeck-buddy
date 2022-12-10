
#pragma once

// system/Qt includes
#include <QObject>

//---------------------------------------------------------------------------------------------------------------------

namespace shared
{
Q_NAMESPACE

//---------------------------------------------------------------------------------------------------------------------

enum class PcState
{
    Normal,
    Restarting,
    ShuttingDown,
};
Q_ENUM_NS(PcState)
}  // namespace shared