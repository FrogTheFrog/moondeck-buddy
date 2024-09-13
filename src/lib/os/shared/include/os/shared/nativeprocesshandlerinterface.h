#pragma once

// system/Qt includes
#include <QRegularExpression>
#include <QString>

namespace os
{
class NativeProcessHandlerInterface
{
public:
    virtual ~NativeProcessHandlerInterface() = default;

    virtual std::vector<uint> getPids() const             = 0;
    virtual QString           getExecPath(uint pid) const = 0;
    virtual void              close(uint pid) const       = 0;
    virtual void              terminate(uint pid) const   = 0;
};
}  // namespace os
