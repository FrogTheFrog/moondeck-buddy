#pragma once

namespace os
{
class AutoStartHandlerInterface
{
public:
    virtual ~AutoStartHandlerInterface() = default;

    virtual void setAutoStart(bool enable)  = 0;
    virtual bool isAutoStartEnabled() const = 0;
};
}  // namespace os
