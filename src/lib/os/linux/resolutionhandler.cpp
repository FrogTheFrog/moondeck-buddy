// header file include
#include "resolutionhandler.h"

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
ResolutionHandler::~ResolutionHandler()
{
    restoreResolution();
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-static)
bool ResolutionHandler::changeResolution(uint width, uint height)
{
    Q_UNUSED(width)
    Q_UNUSED(height)
    qCCritical(lc::os) << Q_FUNC_INFO << ": not implemented!";
    return false;
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-static)
void ResolutionHandler::restoreResolution()
{
    qCCritical(lc::os) << Q_FUNC_INFO << ": not implemented!";
}
}  // namespace os
