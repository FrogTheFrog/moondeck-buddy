#pragma once

// local includes
#include "os/linux/x11resolutionhandler.h"
#include "os/shared/nativeresolutionhandlerinterface.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class NativeResolutionHandler : public NativeResolutionHandlerInterface
{
    Q_DISABLE_COPY(NativeResolutionHandler)

public:
    explicit NativeResolutionHandler() = default;
    ~NativeResolutionHandler() override = default;

    ChangedResMap changeResolution(const DisplayPredicate& predicate) override;

private:
    X11ResolutionHandler x11_handler;
};
}  // namespace os
