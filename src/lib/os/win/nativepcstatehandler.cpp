// header file include
#include "os/win/nativepcstatehandler.h"

// A SEPARATE WINDOWS INCLUDE BECAUSE OF THE SMELL!
#include <windows.h>

// system/Qt includes
#include <QTimer>
#include <powrprof.h>

// local includes
#include "common/loggingcategories.h"

namespace
{
namespace winapi_shim
{
using DeviceNotifyCallbackRoutine = ULONG(NTAPI*)(PVOID context, ULONG type, PVOID setting);

struct DeviceNotifySubscribeParameters
{
    DeviceNotifyCallbackRoutine m_callback;
    PVOID                       m_context;
};

constexpr DWORD DEVICE_NOTIFY_CALLBACK_FLAG{2};

ULONG CALLBACK notifyCallback(PVOID const context, const ULONG type, PVOID /*setting*/)
{
    if (type == PBT_APMRESUMEAUTOMATIC)
    {
        const auto* handler{static_cast<const os::NativePcStateHandler*>(context)};
        QTimer::singleShot(0, handler, &os::NativePcStateHandlerInterface::signalWokeUp);
    }

    return 0;
}
}  // namespace winapi_shim

bool acquirePrivilege()
{
    HANDLE           token_handle{nullptr};
    TOKEN_PRIVILEGES token_privileges;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token_handle) == FALSE)
    {
        qCWarning(lc::os) << "Failed to open process token! Reason: " << lc::getErrorString(GetLastError());
        return false;
    }
    auto cleanup = qScopeGuard([&token_handle]() { CloseHandle(token_handle); });

    {
        token_privileges.PrivilegeCount           = 1;
        token_privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if (LookupPrivilegeValueW(nullptr, SE_SHUTDOWN_NAME, &token_privileges.Privileges[0].Luid) == FALSE)
        {
            qCWarning(lc::os) << "Failed to lookup privilege value! Reason: " << lc::getErrorString(GetLastError());
            return false;
        }
    }

    // Ignoring the useless return value...
    AdjustTokenPrivileges(token_handle, FALSE, &token_privileges, 0, nullptr, nullptr);

    const auto result = GetLastError();
    if (result != ERROR_SUCCESS)
    {
        qCWarning(lc::os) << "Failed to adjust token privileges! Reason: " << lc::getErrorString(result);
        return false;
    }

    return true;
}
}  // namespace

namespace os
{
NativePcStateHandler::NativePcStateHandler()
    : m_privilege_acquired{acquirePrivilege()}
{
    if (!m_privilege_acquired)
    {
        qCWarning(lc::os) << "Failed to acquire shutdown/restart/suspend privilege!";
        return;
    }

    winapi_shim::DeviceNotifySubscribeParameters params{.m_callback = winapi_shim::notifyCallback, .m_context = this};
    m_notify_handle = RegisterSuspendResumeNotification(&params, winapi_shim::DEVICE_NOTIFY_CALLBACK_FLAG);
    if (m_notify_handle == nullptr)
    {
        qCWarning(lc::os) << "Failed to register suspend/resume notification! Reason:"
                          << lc::getErrorString(GetLastError());
    }
}

NativePcStateHandler::~NativePcStateHandler()
{
    if (m_notify_handle)
    {
        UnregisterSuspendResumeNotification(m_notify_handle);
    }
}

bool NativePcStateHandler::canShutdownPC()
{
    return canHandlePc();
}

bool NativePcStateHandler::canRestartPC()
{
    return canHandlePc();
}

bool NativePcStateHandler::canSuspendPC()
{
    return canHandlePc();
}

bool NativePcStateHandler::canHibernatePC()
{
    return canHandlePc();
}

bool NativePcStateHandler::shutdownPC()
{
    if (!canShutdownPC())
    {
        return false;
    }

    if (InitiateSystemShutdownW(nullptr, nullptr, 0, TRUE, FALSE) == FALSE)
    {
        qCWarning(lc::os) << "InitiateSystemShutdownW (shutdown) failed! Reason:" << lc::getErrorString(GetLastError());
        return false;
    }

    return true;
}

bool NativePcStateHandler::restartPC()
{
    if (!canRestartPC())
    {
        return false;
    }

    if (InitiateSystemShutdownW(nullptr, nullptr, 0, TRUE, TRUE) == FALSE)
    {
        qCWarning(lc::os) << "InitiateSystemShutdownW (restart) failed! Reason:" << lc::getErrorString(GetLastError());
        return false;
    }

    return true;
}

bool NativePcStateHandler::suspendPC()
{
    if (!canSuspendPC())
    {
        return false;
    }

    if (SetSuspendState(FALSE, TRUE, FALSE) == FALSE)
    {
        qCWarning(lc::os) << "SetSuspendState failed! Reason:" << lc::getErrorString(GetLastError());
        return false;
    }

    return true;
}

bool NativePcStateHandler::hibernatePC()
{
    if (!canHibernatePC())
    {
        return false;
    }

    if (SetSuspendState(TRUE, TRUE, FALSE) == FALSE)
    {
        qCWarning(lc::os) << "SetSuspendState failed! Reason:" << lc::getErrorString(GetLastError());
        return false;
    }

    return true;
}

bool NativePcStateHandler::canHandlePc() const
{
    return m_privilege_acquired && m_notify_handle != nullptr;
}
}  // namespace os
