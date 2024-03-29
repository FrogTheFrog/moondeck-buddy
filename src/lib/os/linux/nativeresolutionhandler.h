#pragma once

// system/Qt includes
#include <QObject>

// local includes
#include "../nativeresolutionhandlerinterface.h"
#include "x11resolutionhandler.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class NativeResolutionHandler : public NativeResolutionHandlerInterface
{
    Q_DISABLE_COPY(NativeResolutionHandler)

public:
    explicit NativeResolutionHandler()  = default;
    ~NativeResolutionHandler() override = default;

    ChangedResMap changeResolution(const DisplayPredicate& predicate) override;

private:
    X11ResolutionHandler x11_handler;
};
}  // namespace os
