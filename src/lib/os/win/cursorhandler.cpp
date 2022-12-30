// header file include
#include "cursorhandler.h"

// A SEPARATE WINDOWS INCLUDE BECAUSE OF THE SMELL!
#include <windows.h>

// system/Qt includes
#include <limits>

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
QString getError(LSTATUS status)
{
    return QString::fromStdString(std::system_category().message(status));
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
// NOLINTNEXTLINE(*-static)
void CursorHandler::hideCursor()
{
    // Let windows take of the desktop clipping
    if (SetCursorPos(std::numeric_limits<int>::max(), std::numeric_limits<int>::max()) == FALSE)
    {
        qCWarning(lc::os) << "Failed hide the cursor! Reason: " << getError(static_cast<LSTATUS>(GetLastError()));
    }
}
}  // namespace os
