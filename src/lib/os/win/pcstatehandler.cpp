// header file include
#include "pcstatehandler.h"

// A SEPARATE WINDOWS INCLUDE BECAUSE OF THE SMELL!
#include <windows.h>

// system/Qt includes
#include <QProcess>
#include <powrprof.h>

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const int EXTRA_DELAY_SECS{10};
const int SEC_TO_MS{1000};

//---------------------------------------------------------------------------------------------------------------------

QString getError(LSTATUS status)
{
    return QString::fromStdString(std::system_category().message(status));
}

//---------------------------------------------------------------------------------------------------------------------

bool acquireShutdownPrivilege()
{
    HANDLE           token_handle{nullptr};
    TOKEN_PRIVILEGES token_privileges;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token_handle) == FALSE)
    {
        qCWarning(lc::os) << "Failed to open process token! Reason: " << getError(static_cast<LSTATUS>(GetLastError()));
        return false;
    }
    auto cleanup = qScopeGuard([&token_handle]() { CloseHandle(token_handle); });

    {
        token_privileges.PrivilegeCount           = 1;
        token_privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if (LookupPrivilegeValueW(nullptr, SE_SHUTDOWN_NAME, &token_privileges.Privileges[0].Luid) == FALSE)
        {
            qCWarning(lc::os) << "Failed to lookup privilege value! Reason: "
                              << getError(static_cast<LSTATUS>(GetLastError()));
            return false;
        }
    }

    // Ignoring the useless return value...
    AdjustTokenPrivileges(token_handle, FALSE, &token_privileges, 0, nullptr, nullptr);

    const auto result = GetLastError();
    if (result != ERROR_SUCCESS)
    {
        qCWarning(lc::os) << "Failed to adjust token privileges! Reason: " << getError(static_cast<LSTATUS>(result));
        return false;
    }

    return true;
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
PcStateHandler::PcStateHandler()
    : m_privilege_acquired{acquireShutdownPrivilege()}
{
    m_state_change_back_timer.setSingleShot(true);
    connect(&m_state_change_back_timer, &QTimer::timeout, this, &PcStateHandler::slotResetState);
}

//---------------------------------------------------------------------------------------------------------------------

shared::PcState PcStateHandler::getState() const
{
    return m_state;
}

//---------------------------------------------------------------------------------------------------------------------

bool PcStateHandler::shutdownPC(uint grace_period_in_sec)
{
    if (m_state_change_back_timer.isActive())
    {
        qCDebug(lc::os) << "PC is already changing state. Aborting request.";
        return false;
    }

    if (!m_privilege_acquired)
    {
        qCWarning(lc::os) << "Shutdown privilege for the process was not acquired!";
        return false;
    }

    QTimer::singleShot(static_cast<int>(grace_period_in_sec) * SEC_TO_MS, this,
                       [this]()
                       {
                           if (InitiateSystemShutdownW(nullptr, nullptr, 0, TRUE, FALSE) == FALSE)
                           {
                               qCWarning(lc::os) << "Failed to shutdown PC! Reason: "
                                                 << getError(static_cast<LSTATUS>(GetLastError()));
                               slotResetState();
                           }
                       });

    m_state_change_back_timer.start(static_cast<int>(grace_period_in_sec + EXTRA_DELAY_SECS) * SEC_TO_MS);
    m_state = shared::PcState::ShuttingDown;

    return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool PcStateHandler::restartPC(uint grace_period_in_sec)
{
    if (m_state_change_back_timer.isActive())
    {
        qCDebug(lc::os) << "PC is already changing state. Aborting request.";
        return false;
    }

    if (!m_privilege_acquired)
    {
        qCWarning(lc::os) << "Restart privilege for the process was not acquired!";
        return false;
    }

    QTimer::singleShot(static_cast<int>(grace_period_in_sec) * SEC_TO_MS, this,
                       [this]()
                       {
                           if (InitiateSystemShutdownW(nullptr, nullptr, 0, TRUE, TRUE) == FALSE)
                           {
                               qCWarning(lc::os) << "Failed to restart PC! Reason: "
                                                 << getError(static_cast<LSTATUS>(GetLastError()));
                               slotResetState();
                           }
                       });

    m_state_change_back_timer.start(static_cast<int>(grace_period_in_sec + EXTRA_DELAY_SECS) * SEC_TO_MS);
    m_state = shared::PcState::Restarting;

    return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool PcStateHandler::suspendPC(uint grace_period_in_sec)
{
    if (m_state_change_back_timer.isActive())
    {
        qCDebug(lc::os) << "PC is already changing state. Aborting request.";
        return false;
    }

    if (!m_privilege_acquired)
    {
        qCWarning(lc::os) << "Suspend privilege for the process was not acquired!";
        return false;
    }

    QTimer::singleShot(static_cast<int>(grace_period_in_sec) * SEC_TO_MS, this,
                       [this]()
                       {
                           if (SetSuspendState(FALSE, TRUE, FALSE) == FALSE)
                           {
                               qCWarning(lc::os) << "Failed to suspend PC! Reason: "
                                                 << getError(static_cast<LSTATUS>(GetLastError()));
                               slotResetState();
                           }
                       });

    m_state_change_back_timer.start(static_cast<int>(grace_period_in_sec + EXTRA_DELAY_SECS) * SEC_TO_MS);
    m_state = shared::PcState::Suspending;

    return true;
}

//---------------------------------------------------------------------------------------------------------------------

void PcStateHandler::slotResetState()
{
    m_state_change_back_timer.stop();
    m_state = shared::PcState::Normal;
}
}  // namespace os
