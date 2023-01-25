// header file include
#include "nativeprocesshandler.h"

// A SEPARATE WINDOWS INCLUDE BECAUSE OF THE SMELL!
#include <windows.h>

// system/Qt includes
#include <psapi.h>

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
std::vector<uint> NativeProcessHandler::getPids() const
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

        std::vector<uint> pids;
        pids.reserve(handles.size());
        std::copy(asd);

        return pids;
    }

    qFatal("Failed get a list of running processes after 3 tries!");
    return {};
}

//---------------------------------------------------------------------------------------------------------------------

QString NativeProcessHandler::getExecPath(uint pid) const
{
    HANDLE proc_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
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

        DWORD                          data_written{MAX_PATH};
        std::array<char16_t, MAX_PATH> buffer{};

        const BOOL result = QueryFullProcessImageNameW(proc_handle, NULL,
                                                       // NOLINTNEXTLINE(*-reinterpret-cast)
                                                       reinterpret_cast<wchar_t*>(buffer.data()), &data_written);
        if (result == TRUE)
        {
            if (data_written > 0)
            {
                return QString::fromUtf16(buffer.data(), data_written);
            }
        }
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------

void NativeProcessHandler::close(uint pid) const
{
    std::vector<HWND> hwnds;
    {
        HWND hwnd{nullptr};
        do
        {
            DWORD owner_pid{0};

            hwnd = FindWindowExW(nullptr, hwnd, nullptr, nullptr);
            GetWindowThreadProcessId(hwnd, &owner_pid);

            if (static_cast<DWORD>(pid) == owner_pid)
            {
                hwnds.push_back(hwnd);
            }
        } while (hwnd != nullptr);
    }

    for (const auto& hwnd : hwnds)
    {
        if (PostMessageW(hwnd, WM_CLOSE, 0, 0) == FALSE)
        {
            qCDebug(lc::os).nospace() << "Failed to post message to process (pid: " << pid
                                      << ")! Reason: " << lc::getErrorString(GetLastError());
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

void NativeProcessHandler::terminate(uint pid) const
{
    HANDLE proc_handle = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
    auto   cleanup     = qScopeGuard(
        [&]()
        {
            if (proc_handle != nullptr)
            {
                CloseHandle(proc_handle);
            }
        });

    if (proc_handle == nullptr || TerminateProcess(proc_handle, 1) == FALSE)
    {
        qCWarning(lc::os).nospace() << "Failed to terminate process (pid: " << pid
                                    << ")! Reason: " << lc::getErrorString(GetLastError());
    }
}
}  // namespace os
