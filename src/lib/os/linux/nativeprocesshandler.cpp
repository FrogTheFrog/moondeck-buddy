// header file include
#include "nativeprocesshandler.h"

// system/Qt includes
#include <QDir>
#include <QFileInfo>
#include <algorithm>
#include <cerrno>
#include <csignal>
#include <proc/readproc.h>
#include <sys/types.h>

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
uint getParentPid(uint pid)
{
    proc_t proc_info;
    memset(&proc_info, 0, sizeof(proc_info));

    // NOLINTNEXTLINE(*-vararg)
    PROCTAB*   proc = openproc(PROC_FILLSTATUS | PROC_PID, &pid);
    const auto cleanup{qScopeGuard(
        [&]()
        {
            if (proc)
            {
                closeproc(proc);
            }
        })};
    if (readproc(proc, &proc_info) == 0)
    {
        return 0;
    }

    return proc_info.ppid;
}

//---------------------------------------------------------------------------------------------------------------------

std::vector<uint> getPids()
{
    QDir              proc_dir{"/proc"};
    const auto        dirs{proc_dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot)};
    std::vector<uint> pids;

    pids.reserve(static_cast<std::size_t>(dirs.size()));

    for (const auto& dir : dirs)
    {
        bool converted{false};
        uint pid = dir.toUInt(&converted);

        if (!converted)
        {
            continue;
        }

        pids.push_back(pid);
    }

    return pids;
}

//---------------------------------------------------------------------------------------------------------------------

std::vector<uint> getParentPids(const std::vector<uint>& pids)
{
    std::vector<uint> parent_pids;
    std::transform(std::cbegin(pids), std::cend(pids), std::back_inserter(parent_pids), getParentPid);
    return parent_pids;
}

//---------------------------------------------------------------------------------------------------------------------

std::vector<uint> getRelatedPids(uint pid)
{
    std::vector<uint> all_pids{getPids()};
    std::vector<uint> parent_pids{getParentPids(all_pids)};

    Q_ASSERT(all_pids.size() == parent_pids.size());
    std::vector<uint> related_pids;

    // Start searching for children from current pid
    related_pids.push_back(pid);

    for (std::size_t i = 0; i < related_pids.size(); ++i)
    {
        uint related_pid{related_pids[i]};
        for (std::size_t k = 0; k < parent_pids.size(); ++k)
        {
            uint parent_pid{parent_pids[k]};
            if (related_pid != parent_pid)
            {
                continue;
            }

            uint process_pid{all_pids[k]};
            if (process_pid == related_pid)
            {
                // Is this even possible? To be your own parent?
                continue;
            }

            related_pids.push_back(process_pid);
        }
    }

    return related_pids;
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
std::vector<uint> NativeProcessHandler::getPids() const
{
    return ::getPids();
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
    const auto related_pids{getRelatedPids(pid)};
    for (const auto related_pid : related_pids)
    {
        if (kill(static_cast<pid_t>(related_pid), SIGTERM) < 0)
        {
            const auto error{errno};
            if (error != ESRCH)
            {
                qWarning(lc::os) << "Failed to close process" << related_pid << "-" << lc::getErrorString(error);
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

void NativeProcessHandler::terminate(uint pid) const
{
    const auto related_pids{getRelatedPids(pid)};
    for (const auto related_pid : related_pids)
    {
        if (kill(static_cast<pid_t>(related_pid), SIGKILL) < 0)
        {
            const auto error{errno};
            if (error != ESRCH)
            {
                qWarning(lc::os) << "Failed to terminate process" << related_pid << "-" << lc::getErrorString(error);
            }
        }
    }
}
}  // namespace os
