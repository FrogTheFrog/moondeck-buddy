// header file include
#include "processtracker.h"

// A SEPARATE WINDOWS INCLUDE BECAUSE OF THE SMELL!
#include <windows.h>

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
    HANDLE proc_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
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
        HMODULE                        module_handle{nullptr};
        DWORD                          bytes_needed{0};
        std::array<char16_t, MAX_PATH> buffer{};

        // NOLINTNEXTLINE(*-sizeof-expression)
        if (EnumProcessModulesEx(proc_handle, &module_handle, sizeof(module_handle), &bytes_needed,
                                 LIST_MODULES_DEFAULT)
            == TRUE)
        {
            static_assert(sizeof(wchar_t) == sizeof(char16_t), "Wide char is not 2 bytes :/");

            const DWORD written_size =
                GetModuleBaseNameW(proc_handle, module_handle,
                                   // NOLINTNEXTLINE(*-reinterpret-cast)
                                   reinterpret_cast<wchar_t*>(buffer.data()), static_cast<DWORD>(buffer.size()));

            if (written_size > 0)
            {
                return QString::fromUtf16(buffer.data(), written_size);
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
    slotEnumerateProcesses();
}

//---------------------------------------------------------------------------------------------------------------------

bool ProcessTracker::isRunning() const
{
    return m_is_running;
}

//---------------------------------------------------------------------------------------------------------------------

bool ProcessTracker::isRunningNow()
{
    slotEnumerateProcesses();
    return m_is_running;
}

//---------------------------------------------------------------------------------------------------------------------

void ProcessTracker::terminateAll()
{
    const auto pids{enumProcesses()};
    for (const auto pid : pids)
    {
        const auto name{getProcessName(pid)};
        if (name.isEmpty())
        {
            continue;
        }

        if (name.contains(m_name_regex))
        {
            killProcess(pid);
        }
    }

    slotEnumerateProcesses();
}

//---------------------------------------------------------------------------------------------------------------------

void ProcessTracker::slotEnumerateProcesses()
{
    m_update_timer.stop();
    {
        const auto pids{enumProcesses()};
        bool       is_running{false};
        for (const auto pid : pids)
        {
            const auto name{getProcessName(pid)};
            if (name.isEmpty())
            {
                continue;
            }

            if (name.contains(m_name_regex))
            {
                is_running = true;
                break;
            }
        }

        if (m_is_running != is_running)
        {
            if (is_running)
            {
                qDebug("Steam is running!");
            }
            else
            {
                qDebug("Steam is not running!");
            }
            emit signalProcessStateChanged();
        }
        m_is_running = is_running;
    }

    const int timeout_time{5000};
    m_update_timer.start(timeout_time);
}
}  // namespace os
