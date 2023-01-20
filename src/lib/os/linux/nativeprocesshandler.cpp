// header file include
#include "nativeprocesshandler.h"

// system/Qt includes
#include <QFileInfo>
#include <cerrno>
#include <csignal>
#include <sys/types.h>

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
QString NativeProcessHandler::getExecPath(uint pid)
{
    QFileInfo info{"/proc/" + QString::number(pid) + "/exe"};
    return QFileInfo{info.symLinkTarget()}.canonicalFilePath();
}

//---------------------------------------------------------------------------------------------------------------------

void NativeProcessHandler::close(uint pid)
{
    if (kill(static_cast<pid_t>(pid), SIGTERM) < 0)
    {
        const auto error{errno};
        if (error != ESRCH)
        {
            qWarning(lc::os) << "Failed to close process" << pid << "-" << lc::getErrorString(error);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

void NativeProcessHandler::terminate(uint pid)
{
    if (kill(static_cast<pid_t>(pid), SIGKILL) < 0)
    {
        const auto error{errno};
        if (error != ESRCH)
        {
            qWarning(lc::os) << "Failed to terminate process" << pid << "-" << lc::getErrorString(error);
        }
    }
}
}  // namespace os
