#pragma once

// local includes
#include "shared/enums.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class PcStateHandlerInterface
{
public:
    virtual ~PcStateHandlerInterface() = default;

    virtual shared::PcState getState() const = 0;

    virtual bool shutdownPC(uint grace_period_in_sec) = 0;
    virtual bool restartPC(uint grace_period_in_sec)  = 0;
    virtual bool suspendPC(uint grace_period_in_sec)  = 0;
};
}  // namespace os
