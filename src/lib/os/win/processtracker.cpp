// header file include
#include "processtracker.h"

// system/Qt includes
#include <system_error>
#include <unordered_map>

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
QString getError(LSTATUS status)
{
    return QString::fromStdString(std::system_category().message(status));
}

//---------------------------------------------------------------------------------------------------------------------

bool closeProcess(DWORD pid)
{
    std::vector<HWND> hwnds;

    {
        HWND hwnd{nullptr};
        do
        {
            DWORD owner_pid{0};

            hwnd = FindWindowExW(nullptr, hwnd, nullptr, nullptr);
            GetWindowThreadProcessId(hwnd, &owner_pid);

            if (pid == owner_pid)
            {
                hwnds.push_back(hwnd);
            }
        } while (hwnd != nullptr);
    }

    bool message_sent{false};
    for (const auto& hwnd : hwnds)
    {
        if (PostMessageW(hwnd, WM_CLOSE, 0, 0) == FALSE)
        {
            qCDebug(lc::os).nospace() << "Failed to post message to process (pid: " << pid
                                     << ")! Reason: " << getError(GetLastError());
            continue;
        }

        message_sent = true;
    }
    return message_sent;
}

//---------------------------------------------------------------------------------------------------------------------

void killProcess(DWORD pid)
{
    HANDLE proc_handle = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    auto   cleanup     = qScopeGuard(
        [&]()
        {
            if (proc_handle != nullptr)
            {
                CloseHandle(proc_handle);
            }
        });

    if (TerminateProcess(proc_handle, 1) == FALSE)
    {
        qCWarning(lc::os).nospace() << "Failed to terminate process (pid: " << pid
                                   << ")! Reason: " << getError(GetLastError());
    }
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
ProcessTracker::ProcessTracker(QRegularExpression name_regex, std::shared_ptr<ProcessEnumerator> enumerator)
    : m_name_regex{std::move(name_regex)}
    , m_enumerator{std::move(enumerator)}
{
    assert(m_enumerator != nullptr);
    connect(m_enumerator.get(), &ProcessEnumerator::signalProcesses, this, &ProcessTracker::slotUpdateProcessState);
}

//---------------------------------------------------------------------------------------------------------------------

bool ProcessTracker::isRunning() const
{
    return m_pid != 0;
}

//---------------------------------------------------------------------------------------------------------------------

bool ProcessTracker::isRunningNow()
{
    m_enumerator->slotEnumerate();
    return isRunning();
}

//---------------------------------------------------------------------------------------------------------------------

void ProcessTracker::close()
{
    if (isRunningNow())
    {
        if (!closeProcess(m_pid))
        {
            qCDebug(lc::os).nospace() << "No HWND messages were sent, trying to terminate " << m_pid << " instead!";
            killProcess(m_pid);
        }
        m_enumerator->slotEnumerate();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void ProcessTracker::terminate()
{
    if (isRunningNow())
    {
        killProcess(m_pid);
        m_enumerator->slotEnumerate();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void ProcessTracker::slotUpdateProcessState(const std::vector<ProcessEnumerator::ProcessData>& data)
{
    DWORD new_pid{0};
    for (const auto& process : data)
    {
        if (process.m_name.contains(m_name_regex))
        {
            new_pid = process.m_pid;
            break;
        }
    }

    if (m_pid != new_pid)
    {
        m_pid = new_pid;
        emit signalProcessStateChanged();
    }
}
}  // namespace os
