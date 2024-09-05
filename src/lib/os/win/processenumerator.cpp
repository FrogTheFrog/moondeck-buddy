// header file include
#include "os/win/processenumerator.h"

// system/Qt includes
#include <psapi.h>
#include <unordered_map>

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
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

        const BOOL result = QueryFullProcessImageNameW(proc_handle, 0,
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
            qFatal("Failed get a list of running processes! Reason: %s",
                   qUtf8Printable(lc::getErrorString(GetLastError())));
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
ProcessEnumerator::ProcessEnumerator()
{
    connect(&m_timer, &QTimer::timeout, this, &ProcessEnumerator::slotEnumerate);
    m_timer.setSingleShot(true);
}

//---------------------------------------------------------------------------------------------------------------------

void ProcessEnumerator::start(uint interval_ms)
{
    m_started = true;
    m_timer.setInterval(static_cast<int>(interval_ms));
    slotEnumerate();
}

//---------------------------------------------------------------------------------------------------------------------

void ProcessEnumerator::stop()
{
    m_started = false;
    m_timer.stop();
}

//---------------------------------------------------------------------------------------------------------------------

void ProcessEnumerator::slotEnumerate()
{
    m_timer.stop();

    std::vector<ProcessData> data;
    const auto               pids{enumProcesses()};

    for (const auto pid : pids)
    {
        const auto name{getProcessName(pid)};
        if (name.isEmpty())
        {
            continue;
        }

        data.push_back(ProcessData{pid, name});
    }

    emit signalProcesses(data);

    if (m_started)
    {
        m_timer.start();
    }
}
}  // namespace os
