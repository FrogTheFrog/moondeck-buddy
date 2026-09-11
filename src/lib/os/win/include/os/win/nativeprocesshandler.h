#pragma once

// local includes
#include "os/common/nativeprocesshandlerinterface.h"

namespace os
{
class NativeProcessHandler : public NativeProcessHandlerInterface
{
    Q_DISABLE_COPY(NativeProcessHandler)

public:
    explicit NativeProcessHandler()  = default;
    ~NativeProcessHandler() override = default;

    std::vector<uint>        getPids() const override;
    std::optional<QString>   getExecPath(uint pid) const override;
    std::optional<QDateTime> getStartTime(uint pid) const override;
    std::optional<bool>      close(uint pid) const override;
    std::optional<bool>      terminate(uint pid) const override;
};
}  // namespace os
