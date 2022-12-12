// header file include
#include "processtracker.h"

// system/Qt includes
#include <psapi.h>
#include <system_error>
#include <unordered_map>

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
            qDebug("Failed to post message to process (pid: %lu)! Reason: %s", pid,
                   qUtf8Printable(getError(GetLastError())));
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
        qWarning("Failed to terminate process (pid: %lu)! Reason: %s", pid, qUtf8Printable(getError(GetLastError())));
    }
}

//---------------------------------------------------------------------------------------------------------------------

QString getProcessName(DWORD pid)
{
    HANDLE proc_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    auto   cleanup     = qScopeGuard(
        [&]()
        {
            if (proc_handle != nullptr)
            {
                CloseHandle(proc_handle);
            }
        });

    if (proc_handle != nullptr)
    {
        static_assert(sizeof(wchar_t) == sizeof(char16_t), "Wide char is not 2 bytes :/");

        DWORD                          bytes_written{MAX_PATH};
        std::array<char16_t, MAX_PATH> buffer{};

        const BOOL result = QueryFullProcessImageNameW(proc_handle, NULL,
                                                       // NOLINTNEXTLINE(*-reinterpret-cast)
                                                       reinterpret_cast<wchar_t*>(buffer.data()), &bytes_written);
        if (result == TRUE)
        {
            if (bytes_written > 0)
            {
                return QString::fromUtf16(buffer.data(), bytes_written);
            }
        }
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------

std::vector<DWORD> enumProcesses()
{
    std::vector<DWORD> handles;
    for (auto i = 0; i < 3; ++i)
    {
        const int handle_size{1024};
        // NOLINTNEXTLINE(*misplaced-widening-cast)
        handles.resize(static_cast<std::size_t>(handle_size * (i + 1)));

        DWORD       bytes_needed{0};
        const DWORD buffer_in_bytes{static_cast<DWORD>(handles.size() * sizeof(decltype(handles)::value_type))};

        if (EnumProcesses(handles.data(), buffer_in_bytes, &bytes_needed) == FALSE)
        {
            qFatal("Failed get a list of running processes! Reason: %s", qUtf8Printable(getError(GetLastError())));
        }

        if (buffer_in_bytes == bytes_needed)
        {
            continue;
        }

        handles.resize(bytes_needed / sizeof(decltype(handles)::value_type));
        return handles;
    }

    qFatal("Failed get a list of running processes after 3 tries!");
    return {};
}

}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
ProcessTracker::ProcessTracker(QRegularExpression name_regex)
    : m_name_regex{std::move(name_regex)}
{
    connect(&m_update_timer, &QTimer::timeout, this, &ProcessTracker::slotEnumerateProcesses);

    m_update_timer.setSingleShot(true);
}

//---------------------------------------------------------------------------------------------------------------------

void ProcessTracker::startObserving()
{
    slotEnumerateProcesses();
    if (!isRunning())
    {
        emit signalProcessStateChanged(false);
    }
}

//---------------------------------------------------------------------------------------------------------------------

bool ProcessTracker::isRunning() const
{
    return m_pid != 0;
}

//---------------------------------------------------------------------------------------------------------------------

bool ProcessTracker::isRunningNow()
{
    slotEnumerateProcesses();
    return isRunning();
}

//---------------------------------------------------------------------------------------------------------------------

void ProcessTracker::close()
{
    if (isRunningNow())
    {
        if (!closeProcess(m_pid))
        {
            qDebug("No HWND messages were sent, trying to terminate %lu instead!", m_pid);
            killProcess(m_pid);
        }
        slotEnumerateProcesses();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void ProcessTracker::terminate()
{
    if (isRunningNow())
    {
        killProcess(m_pid);
        slotEnumerateProcesses();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void ProcessTracker::slotEnumerateProcesses()
{
    m_update_timer.stop();
    {
        const auto pids{enumProcesses()};
        DWORD      new_pid{0};
        for (const auto pid : pids)
        {
            const auto name{getProcessName(pid)};
            if (name.isEmpty())
            {
                continue;
            }

            if (name.contains(m_name_regex))
            {
                new_pid = pid;
                break;
            }
        }

        if (m_pid != new_pid)
        {
            m_pid = new_pid;
            emit signalProcessStateChanged(isRunning());
        }
    }

    const int timeout_time{2500};
    m_update_timer.start(timeout_time);
}
}  // namespace os
