// header file include
#include "os/win/nativeprocesshandler.h"

// A SEPARATE WINDOWS INCLUDE BECAUSE OF THE SMELL!
#include <windows.h>

// system/Qt includes
#include <psapi.h>

// local includes
#include "shared/loggingcategories.h"

#include <QTimeZone>

namespace
{
template<class Getter>
auto useProcHandle(const uint pid, Getter&& getter)
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

    return getter(proc_handle);
}
}  // namespace

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
        std::transform(std::cbegin(handles), std::cend(handles), std::back_inserter(pids),
                       [](const DWORD handle) { return static_cast<uint>(handle); });
        return pids;
    }

    qFatal("Failed get a list of running processes after 3 tries!");
    return {};
}

QString NativeProcessHandler::getExecPath(uint pid) const
{
    return useProcHandle(pid,
                         [](HANDLE handle)
                         {
                             static_assert(sizeof(wchar_t) == sizeof(char16_t), "Wide char is not 2 bytes :/");

                             if (!handle)
                             {
                                 return QString{};
                             }

                             DWORD                          data_written{MAX_PATH};
                             std::array<char16_t, MAX_PATH> buffer{};

                             const BOOL result =
                                 QueryFullProcessImageNameW(handle, 0,
                                                            // NOLINTNEXTLINE(*-reinterpret-cast)
                                                            reinterpret_cast<wchar_t*>(buffer.data()), &data_written);
                             if (result == TRUE)
                             {
                                 if (data_written > 0)
                                 {
                                     return QString::fromUtf16(buffer.data(), data_written);
                                 }
                             }

                             qDebug(lc::os)
                                 << "QueryFullProcessImageNameW failed: " << lc::getErrorString(GetLastError());
                             return QString{};
                         });
}

QDateTime NativeProcessHandler::getStartTime(uint pid) const
{
    return useProcHandle(pid,
                         [](HANDLE handle)
                         {
                             static_assert(sizeof(wchar_t) == sizeof(char16_t), "Wide char is not 2 bytes :/");

                             if (!handle)
                             {
                                 return QDateTime{};
                             }

                             FILETIME start_time{};
                             FILETIME exit_time{};
                             FILETIME kernel_time{};
                             FILETIME user_time{};
                             BOOL result = GetProcessTimes(handle, &start_time, &exit_time, &kernel_time, &user_time);

                             if (result == TRUE)
                             {
                                 SYSTEMTIME time_utc{};
                                 result = FileTimeToSystemTime(&start_time, &time_utc);
                                 if (result == TRUE)
                                 {
                                     const QDateTime datetime{QDate{time_utc.wYear, time_utc.wMonth, time_utc.wDay},
                                                              QTime{time_utc.wHour, time_utc.wMinute, time_utc.wSecond,
                                                                    time_utc.wMilliseconds},
                                                              QTimeZone::UTC};
                                     return datetime.toLocalTime();
                                 }

                                 qDebug(lc::os)
                                     << "FileTimeToSystemTime failed: " << lc::getErrorString(GetLastError());
                             }
                             else
                             {
                                 qDebug(lc::os) << "GetProcessTimes failed: " << lc::getErrorString(GetLastError());
                             }

                             return QDateTime{};
                         });
}

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
