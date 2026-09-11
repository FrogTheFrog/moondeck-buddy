// header file include
#include "os/processreaper.h"

// local includes
#include "common/loggingcategories.h"
#include "os/processhandler.h"

namespace
{
const int INTERVAL_MS{1000};
const int FORCED_TERMINATE_ON_COUNT{3};
const int MAX_LOOP{FORCED_TERMINATE_ON_COUNT * 2};
}  // namespace

namespace os
{
ProcessReaper::PidsWithData ProcessReaper::getCurrentTimestamps(const std::set<uint>& pids, bool with_exec_path)
{
    const ProcessHandler process_handler;

    PidsWithData output;
    for (const auto& pid : pids)
    {
        const auto& timestamp{process_handler.getStartTime(pid)};
        if (!timestamp)
        {
            qCWarning(lc::os) << "failed to retrieve timestamp for PID" << pid;
            continue;
        }

        std::optional<QString> exec_path;
        if (with_exec_path)
        {
            exec_path = process_handler.getExecPath(pid);
            if (!exec_path)
            {
                qCWarning(lc::os) << "failed to retrieve exec path for PID" << pid;
                continue;
            }
        }

        output[pid] = PidData{.m_timestamp = *timestamp, .m_exec_path = exec_path};
    }

    return output;
}

ProcessReaper::ProcessReaper(const std::set<uint>& pids)
    : ProcessReaper(getCurrentTimestamps(pids))
{
    if (m_pids_with_data.size() != pids.size())
    {
        qCWarning(lc::os) << "Failed to retrieve timestamps for all PIDs. This could happen if they died or we do not "
                             "have access. They will be skipped!";
    }
}

ProcessReaper::ProcessReaper(PidsWithData pids_with_data)
    : m_pids_with_data{std::move(pids_with_data)}
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(INTERVAL_MS);
}

bool ProcessReaper::start()
{
    if (m_already_reaping)
    {
        qCWarning(lc::os) << "Reaper has already been started!";
        return false;
    }

    for (const auto& [pid, data] : m_pids_with_data)
    {
        if (!data.m_timestamp.isValid())
        {
            qCWarning(lc::os) << "Supplied timestamp for PID" << pid << "is invalid! Reaper will not start.";
            return false;
        }
    }

    m_already_reaping = true;
    m_timer.start();
    return true;
}

void ProcessReaper::slotPerformReaping()
{
    const ProcessHandler process_handler;

    for (auto it{std::begin(m_pids_with_data)}; it != std::end(m_pids_with_data);)
    {
        const auto& [pid, data] = *it;

        // Invalid or different timestamp means that we probably no longer have the same PID.
        if (const auto& current_timestamp{process_handler.getStartTime(pid)};
            current_timestamp && current_timestamp == data.m_timestamp)
        {
            const bool try_close{m_repeat_counter == 0};
            bool       try_terminate{m_repeat_counter == FORCED_TERMINATE_ON_COUNT};

            if (try_close && process_handler.close(pid) != true)
            {
                qCDebug(lc::os) << "Failed to invoke 'close' for PID" << pid;
                try_terminate = true;
            }

            if (try_terminate && process_handler.terminate(pid) != true)
            {
                qCDebug(lc::os) << "Failed to invoke 'terminate' for PID" << pid;
                // We cannot do anything to this PID.
            }

            continue;
        }

        it = m_pids_with_data.erase(it);
    }

    if (m_pids_with_data.empty() || m_repeat_counter == MAX_LOOP)
    {
        emit signalFinishedReaping(m_pids_with_data);
        return;
    }

    ++m_repeat_counter;
    m_timer.start();
}
}  // namespace os
