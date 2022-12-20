#pragma once

// system/Qt includes
#include <QJsonDocument>

// local includes
#include "shared/enums.h"

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::out
{
struct PcState
{
    static QJsonDocument toJson(const PcState& data);

    shared::PcState m_pc_state;
};
}  // namespace msgs::out
