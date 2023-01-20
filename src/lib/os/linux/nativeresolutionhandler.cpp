// header file include
#include "nativeresolutionhandler.h"

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
NativeResolutionHandler::ChangedResMap NativeResolutionHandler::changeResolution(const DisplayPredicate& predicate)
{
    Q_UNUSED(predicate)
    qCCritical(lc::os) << Q_FUNC_INFO << ": not implemented!";
    return {};
}
}  // namespace os
