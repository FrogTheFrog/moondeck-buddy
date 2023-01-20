// header file include
#include "nativepcstatehandler.h"

// A SEPARATE WINDOWS INCLUDE BECAUSE OF THE SMELL!
#include <windows.h>

// system/Qt includes
#include <powrprof.h>

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
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

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
NativePcStateHandler::NativePcStateHandler()
    : m_privilege_acquired{acquirePrivilege()}
{
    if (!m_privilege_acquired)
    {
        qCWarning(lc::os) << "failed to acquire shutdown/restart/suspend privilege!";
    }
}

//---------------------------------------------------------------------------------------------------------------------

bool NativePcStateHandler::canShutdownPC()
{
    return m_privilege_acquired;
}

//---------------------------------------------------------------------------------------------------------------------

bool NativePcStateHandler::canRestartPC()
{
    return m_privilege_acquired;
}

//---------------------------------------------------------------------------------------------------------------------

bool NativePcStateHandler::canSuspendPC()
{
    return m_privilege_acquired;
}

//---------------------------------------------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------------------------------------------

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
}  // namespace os
