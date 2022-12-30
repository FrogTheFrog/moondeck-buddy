#pragma once

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class CursorHandlerInterface
{
public:
    virtual ~CursorHandlerInterface() = default;

    virtual void hideCursor() = 0;
};
}  // namespace os
