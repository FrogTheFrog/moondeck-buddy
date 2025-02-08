// header file include
#include "os/linux/nativeprocesshandler.h"

// system/Qt includes
#include <QDir>
#include <QFileInfo>
#include <algorithm>
#include <cerrno>
#include <csignal>
#include <libproc2/pids.h>
#include <libproc2/stat.h>
#include <sys/types.h>

// local includes
#include "shared/loggingcategories.h"

namespace
{
std::optional<unsigned int> getBootTime()
{
    static unsigned int boot_time{0};
    if (boot_time == 0)
    {
        stat_info* stat_info{nullptr};
        if (const int error = procps_stat_new(&stat_info); error < 0)
        {
            qWarning(lc::os) << "Failed at procps_stat_new: " << lc::getErrorString(error * -1);
            return std::nullopt;
        }

        boot_time = STAT_GET(stat_info, STAT_SYS_TIME_OF_BOOT, ul_int);
        procps_stat_unref(&stat_info);
    }
    return boot_time;
}

template<class ItemValue, class Getter>
ItemValue getPidItem(const uint pid, const pids_item item, const ItemValue& fallback, Getter&& getter)
{
    std::array items{item};

    pids_info* info{nullptr};
    if (const int error = procps_pids_new(&info, items.data(), items.size()); error < 0)
    {
        qWarning(lc::os) << "Failed at procps_pids_new for" << pid << "-" << lc::getErrorString(error * -1);
        return fallback;
    }

    const auto cleanup{qScopeGuard([&]() { procps_pids_unref(&info); })};

    std::array  pids{pid};
    const auto* result{procps_pids_select(info, pids.data(), pids.size(), PIDS_SELECT_PID)};

    if (!result)
    {
        qWarning(lc::os) << "Failed at procps_pids_select for" << pid << "-" << lc::getErrorString(errno);
        return fallback;
    }

    if (!result->counts || result->counts->total <= 0)
    {
        return fallback;
    }

    const auto* head_ptr{result->stacks && result->stacks[0]->head ? result->stacks[0]->head : nullptr};
    if (!head_ptr)
    {
        qWarning(lc::os) << "Failed at procps_pids_select for" << pid << "-" << lc::getErrorString(errno);
        return fallback;
    }

    return std::forward<Getter>(getter)(head_ptr->result);
}

uint getParentPid(const uint pid)
{
    return getPidItem(pid, PIDS_ID_PPID, 0u,
                      [](const auto& result) { return result.s_int >= 0 ? static_cast<uint>(result.s_int) : 0u; });
}

QString getCmdline(const uint pid)
{
    return getPidItem(pid, PIDS_CMDLINE_V, QString{},
                      [](const auto& result)
                      {
                          auto* ptr_list{result.strv};
                          if (ptr_list == nullptr)
                          {
                              return QString{};
                          }

                          QStringList cmdline;
                          while (*ptr_list)
                          {
                              cmdline.append(*ptr_list);
                              // NOLINTNEXTLINE(*-pointer-arithmetic)
                              ++ptr_list;
                          }

                          return cmdline.join(' ');
                      });
}

QDateTime getStartTime(const uint pid)
{
    return getPidItem(pid, PIDS_TIME_START, QDateTime{},
                      [](const auto& result)
                      {
                          const auto boot_time{getBootTime()};
                          if (!boot_time)
                          {
                              return QDateTime{};
                          }

                          const auto milliseconds{static_cast<int>(std::round((result.real) * 1000.0))};
                          const auto datetime{QDateTime::fromSecsSinceEpoch(*boot_time)};
                          return datetime.addMSecs(milliseconds);
                      });
}

std::vector<uint> getPids()
{
    const QDir        proc_dir{"/proc"};
    const auto        dirs{proc_dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot)};
    std::vector<uint> pids;

    pids.reserve(dirs.size());

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

std::vector<uint> getParentPids(const std::vector<uint>& pids)
{
    std::vector<uint> parent_pids;
    std::transform(std::cbegin(pids), std::cend(pids), std::back_inserter(parent_pids), getParentPid);
    return parent_pids;
}

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

namespace os
{
std::vector<uint> NativeProcessHandler::getPids() const
{
    return ::getPids();
}

QString NativeProcessHandler::getExecPath(uint pid) const
{
    const QFileInfo info{"/proc/" + QString::number(pid) + "/exe"};
    return QFileInfo{info.symLinkTarget()}.canonicalFilePath();
}

QDateTime NativeProcessHandler::getStartTime(uint pid) const
{
    return ::getStartTime(pid);
}

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

    const auto search_pids = [&all_pids, &parent_pids](uint parent_needle_pid)
    {
        std::vector<uint> children_pids;
        for (std::size_t i = 0; i < all_pids.size(); ++i)
        {
            const uint parent_pid{parent_pids[i]};
            if (parent_pid != parent_needle_pid)
            {
                continue;
            }

            const uint process_pid{all_pids[i]};
            if (process_pid == parent_needle_pid)
            {
                // Is this even possible? To be your own parent?
                continue;
            }

            children_pids.push_back(process_pid);
        }

        return children_pids;
    };

    std::vector<uint> children_pids{search_pids(pid)};
    std::vector<uint> nested_children_pids{children_pids};
    for (const auto child_pid : children_pids)
    {
        std::vector<uint> nested_pids{search_pids(child_pid)};
        std::copy(std::begin(nested_pids), std::end(nested_pids), std::back_inserter(nested_children_pids));
    }

    std::sort(std::begin(nested_children_pids), std::end(nested_children_pids));
    auto last_unique = std::unique(std::begin(nested_children_pids), std::end(nested_children_pids));
    nested_children_pids.erase(last_unique, std::end(nested_children_pids));

    return nested_children_pids;
}

// NOLINTNEXTLINE(*-to-static)
QString NativeProcessHandler::getCmdline(uint pid) const
{
    return ::getCmdline(pid);
}
}  // namespace os
