#pragma once

// system/Qt includes
#include <QObject>

// local includes
#include "os/shared/nativeresolutionhandlerinterface.h"

namespace os
{
class X11ResolutionHandler : public NativeResolutionHandlerInterface
{
    Q_DISABLE_COPY(X11ResolutionHandler)

public:
    explicit X11ResolutionHandler()  = default;
    ~X11ResolutionHandler() override = default;

    ChangedResMap changeResolution(const DisplayPredicate& predicate) override;
};
}  // namespace os
