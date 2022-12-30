// header file include
#include "autostarthandler.h"

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
// NOLINTNEXTLINE(*-static)
void AutoStartHandler::setAutoStart(bool enable)
{
    Q_UNUSED(enable)
    qCCritical(lc::os) << Q_FUNC_INFO << ": not implemented!";
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-static)
bool AutoStartHandler::isAutoStartEnabled() const
{
    qCCritical(lc::os) << Q_FUNC_INFO << ": not implemented!";
    return false;
}
}  // namespace os
