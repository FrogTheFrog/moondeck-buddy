// header file include
#include "cursorhandler.h"

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
// NOLINTNEXTLINE(*-static)
void CursorHandler::hideCursor()
{
    qCCritical(lc::os) << Q_FUNC_INFO << ": not implemented!";
}
}  // namespace os
