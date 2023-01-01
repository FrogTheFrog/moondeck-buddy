#pragma once

// system/Qt includes
#include <QtGlobal>

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class ResolutionHandlerInterface
{
public:
    virtual ~ResolutionHandlerInterface() = default;

    virtual bool changeResolution(uint width, uint height) = 0;
    virtual void restoreResolution()                       = 0;
};
}  // namespace os
