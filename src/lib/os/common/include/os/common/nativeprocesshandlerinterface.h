#pragma once

// system/Qt includes
#include <QDateTime>
#include <QString>

namespace os
{
class NativeProcessHandlerInterface
{
public:
    virtual ~NativeProcessHandlerInterface() = default;

    virtual std::vector<uint>        getPids() const              = 0;
    virtual std::optional<QString>   getExecPath(uint pid) const  = 0;
    virtual std::optional<QDateTime> getStartTime(uint pid) const = 0;
    virtual std::optional<bool>      close(uint pid) const        = 0;
    virtual std::optional<bool>      terminate(uint pid) const    = 0;
};
}  // namespace os
