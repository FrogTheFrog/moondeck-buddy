// header file include
#include "nativeprocesshandler.h"

// system/Qt includes
#include <QDir>
#include <QFileInfo>
#include <cerrno>
#include <csignal>
#include <sys/types.h>

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
std::vector<uint> NativeProcessHandler::getPids() const
{
    QDir              proc_dir{"/proc"};
    const auto        dirs{proc_dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot)};
    std::vector<uint> matching_pids;

    matching_pids.reserve(static_cast<std::size_t>(dirs.size()));

    for (const auto& dir : dirs)
    {
        bool converted{false};
        uint pid = dir.toUInt(&converted);

        if (!converted)
        {
            continue;
        }

        matching_pids.push_back(pid);
    }

    return matching_pids;
}

//---------------------------------------------------------------------------------------------------------------------

QString NativeProcessHandler::getExecPath(uint pid) const
{
    QFileInfo info{"/proc/" + QString::number(pid) + "/exe"};
    return QFileInfo{info.symLinkTarget()}.canonicalFilePath();
}

//---------------------------------------------------------------------------------------------------------------------

void NativeProcessHandler::close(uint pid) const
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

void NativeProcessHandler::terminate(uint pid) const
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
