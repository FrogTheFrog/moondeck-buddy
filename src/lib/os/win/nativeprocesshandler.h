#pragma once

// local includes
#include "../nativeprocesshandlerinterface.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class NativeProcessHandler : public NativeProcessHandlerInterface
{
    Q_DISABLE_COPY(NativeProcessHandler)

public:
    explicit NativeProcessHandler()  = default;
    ~NativeProcessHandler() override = default;

    QString getExecPath(uint pid) override;
    void    close(uint pid) override;
    void    terminate(uint pid) override;
};
}  // namespace os
