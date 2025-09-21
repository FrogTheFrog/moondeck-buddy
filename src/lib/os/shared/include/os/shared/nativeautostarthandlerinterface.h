#pragma once

namespace os
{
class NativeAutoStartHandlerInterface
{
public:
    virtual ~NativeAutoStartHandlerInterface() = default;

    virtual void setAutoStart(bool enable)  = 0;
    virtual bool isAutoStartEnabled() const = 0;

    virtual bool isServiceSupported() const = 0;
    virtual bool restartIntoService()       = 0;
};
}  // namespace os
