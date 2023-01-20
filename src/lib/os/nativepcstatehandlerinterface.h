#pragma once

// local includes
#include "shared/enums.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class NativePcStateHandlerInterface
{
public:
    virtual ~NativePcStateHandlerInterface() = default;

    virtual bool canShutdownPC() = 0;
    virtual bool canRestartPC()  = 0;
    virtual bool canSuspendPC()  = 0;

    virtual bool shutdownPC() = 0;
    virtual bool restartPC()  = 0;
    virtual bool suspendPC()  = 0;
};
}  // namespace os
