#pragma once

// system/Qt includes
#include <QString>

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class NativeProcessHandlerInterface
{
public:
    virtual ~NativeProcessHandlerInterface() = default;

    virtual QString getExecPath(uint pid) = 0;
    virtual void    close(uint pid)       = 0;
    virtual void    terminate(uint pid)   = 0;
};
}  // namespace os
