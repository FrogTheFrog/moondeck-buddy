#pragma once

// local includes
#include "os/shared/nativeprocesshandlerinterface.h"

namespace os
{
class NativeProcessHandler : public NativeProcessHandlerInterface
{
    Q_DISABLE_COPY(NativeProcessHandler)

public:
    explicit NativeProcessHandler()  = default;
    ~NativeProcessHandler() override = default;

    std::vector<uint> getPids() const override;
    QString           getExecPath(uint pid) const override;
    QDateTime         getStartTime(uint pid) const override;
    void              close(uint pid) const override;
    void              terminate(uint pid) const override;

    std::vector<uint> getChildrenPids(uint pid) const;  // TODO: maybe remove
    QString           getCmdline(uint pid) const;       // TODO: maybe remove
};
}  // namespace os
