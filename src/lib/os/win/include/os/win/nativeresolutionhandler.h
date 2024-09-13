#pragma once

// system/Qt includes
#include <QObject>

// local includes
#include "os/shared/nativeresolutionhandlerinterface.h"

namespace os
{
class NativeResolutionHandler : public NativeResolutionHandlerInterface
{
    Q_DISABLE_COPY(NativeResolutionHandler)

public:
    explicit NativeResolutionHandler()  = default;
    ~NativeResolutionHandler() override = default;

    ChangedResMap changeResolution(const DisplayPredicate& predicate) override;
};
}  // namespace os
