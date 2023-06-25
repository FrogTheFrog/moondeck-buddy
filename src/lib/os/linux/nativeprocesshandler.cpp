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
    const std::array<uint, 2> pids{pid, 0};

    proc_t proc_info;
    memset(&proc_info, 0, sizeof(proc_info));

    // NOLINTNEXTLINE(*-vararg)
    PROCTAB*   proc = openproc(PROC_FILLSTATUS | PROC_PID, pids.data());
    const auto cleanup{qScopeGuard(
        [&]()
        {
            if (proc != nullptr)
            {
                closeproc(proc);
            }
        })};
    if (readproc(proc, &proc_info) == nullptr)
    {
        return 0;
    }

    return proc_info.ppid;
}

//---------------------------------------------------------------------------------------------------------------------

QString getCmdline(uint pid)
{
    const std::array<uint, 2> pids{pid, 0};

    proc_t proc_info;
    memset(&proc_info, 0, sizeof(proc_info));

    // NOLINTNEXTLINE(*-vararg)
    PROCTAB*   proc = openproc(PROC_FILLCOM | PROC_PID, pids.data());
    const auto cleanup{qScopeGuard(
        [&]()
        {
            if (proc != nullptr)
            {
                closeproc(proc);
            }
        })};
    if (readproc(proc, &proc_info) == nullptr)
    {
        return {};
    }

    auto* ptr_list{proc_info.cmdline};
    if (ptr_list == nullptr)
    {
        return {};
    }

    QStringList cmdline;
    while (*ptr_list != nullptr)
    {
        cmdline.append(*ptr_list);
        ptr_list++;
    }

    return cmdline.join(' ');
}

//---------------------------------------------------------------------------------------------------------------------

std::vector<uint> getPids()
{
    const QDir        proc_dir{"/proc"};
    const auto        dirs{proc_dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot)};
    std::vector<uint> pids;

    pids.reserve(static_cast<std::size_t>(dirs.size()));

    for (const auto& dir : dirs)
    {
        bool       converted{false};
        const uint pid = dir.toUInt(&converted);

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
    const std::vector<uint> all_pids{getPids()};
    const std::vector<uint> parent_pids{getParentPids(all_pids)};

    Q_ASSERT(all_pids.size() == parent_pids.size());
    std::vector<uint> related_pids;

    // Start searching for children from current pid
    related_pids.push_back(pid);

    for (std::size_t i = 0; i < related_pids.size(); ++i)
    {
        const uint related_pid{related_pids[i]};
        for (std::size_t k = 0; k < parent_pids.size(); ++k)
        {
            const uint parent_pid{parent_pids[k]};
            if (related_pid != parent_pid)
            {
                continue;
            }

            const uint process_pid{all_pids[k]};
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
    const QFileInfo info{"/proc/" + QString::number(pid) + "/exe"};
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

//---------------------------------------------------------------------------------------------------------------------

std::vector<uint> NativeProcessHandler::getChildrenPids(uint pid) const
{
    const std::vector<uint> all_pids{getPids()};
    if (std::find(std::begin(all_pids), std::end(all_pids), pid) == std::end(all_pids))
    {
        // Process does not exist, early exit.
        return {};
    }

    const std::vector<uint> parent_pids{getParentPids(all_pids)};
    Q_ASSERT(all_pids.size() == parent_pids.size());

    std::vector<uint> children_pids;
    for (std::size_t i = 0; i < all_pids.size(); ++i)
    {
        const uint parent_pid{parent_pids[i]};
        if (parent_pid != pid)
        {
            continue;
        }

        const uint process_pid{all_pids[i]};
        if (process_pid == pid)
        {
            // Is this even possible? To be your own parent?
            continue;
        }

        children_pids.push_back(process_pid);
    }

    return children_pids;
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-to-static)
QString NativeProcessHandler::getCmdline(uint pid) const
{
    return ::getCmdline(pid);
}
}  // namespace os
