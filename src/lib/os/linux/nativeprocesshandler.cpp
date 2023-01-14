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

namespace
{
QString getError(int status)
{
    return QString::fromStdString(std::system_category().message(status));
}
}  // namespace

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
    if (kill(pid, SIGTERM) < 0)
    {
        const auto error{errno};
        if (error != ESRCH)
        {
            qWarning(lc::os) << "Failed to close process" << pid << "-" << getError(error);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

void NativeProcessHandler::terminate(uint pid)
{
    if (kill(pid, SIGKILL) < 0)
    {
        const auto error{errno};
        if (error != ESRCH)
        {
            qWarning(lc::os) << "Failed to terminate process" << pid << "-" << getError(error);
        }
    }
}
}  // namespace os
