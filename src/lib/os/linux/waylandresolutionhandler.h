#pragma once

// system/Qt includes
#include <QObject>

// local includes
#include "../nativeresolutionhandlerinterface.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class WaylandResolutionHandler : public NativeResolutionHandlerInterface
{
    Q_DISABLE_COPY(WaylandResolutionHandler)

public:
    explicit WaylandResolutionHandler()  = default;
    ~WaylandResolutionHandler() override = default;

    ChangedResMap changeResolution(const DisplayPredicate& predicate) override;
};
}  // namespace os
