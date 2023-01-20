// header file include
#include "processhandler.h"

// local includes
#include "shared/loggingcategories.h"

// local includes
#include "nativeprocesshandlerinterface.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
bool validProcessExists(const QString& exec_path, const QRegularExpression& exec_regex)
{
    if (exec_path.isEmpty())
    {
        return false;
    }

    return exec_path.contains(exec_regex);
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
ProcessHandler::ProcessHandler(std::unique_ptr<NativeProcessHandlerInterface> native_handler)
    : m_native_handler{std::move(native_handler)}
{
    Q_ASSERT(m_native_handler != nullptr);

    connect(&m_check_timer, &QTimer::timeout, this, &ProcessHandler::slotCheckState);
    connect(&m_kill_timer, &QTimer::timeout, this, [this]() { terminate(); });

    const int check_inverval{1000};
    m_check_timer.setInterval(check_inverval);
    m_check_timer.setSingleShot(true);
    m_kill_timer.setSingleShot(true);
}

//---------------------------------------------------------------------------------------------------------------------

// For forward declarations
ProcessHandler::~ProcessHandler() = default;

//---------------------------------------------------------------------------------------------------------------------

bool ProcessHandler::startMonitoring(uint pid, const QRegularExpression& exec_regex)
{
    stopMonitoring();

    if (pid == 0)
    {
        return false;
    }

    if (pid == m_pid)
    {
        return true;
    }

    if (!validProcessExists(m_native_handler->getExecPath(pid), exec_regex))
    {
        return false;
    }

    m_pid        = pid;
    m_exec_regex = exec_regex;
    m_check_timer.start();
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

void ProcessHandler::stopMonitoring()
{
    m_pid        = 0;
    m_exec_regex = {};
    m_check_timer.stop();
    m_kill_timer.stop();
}

//---------------------------------------------------------------------------------------------------------------------

void ProcessHandler::close(std::optional<uint> auto_termination_timer)
{
    if (isRunningNow())
    {
        m_native_handler->close(m_pid);
        if (auto_termination_timer)
        {
            m_kill_timer.start(static_cast<int>(*auto_termination_timer));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

void ProcessHandler::terminate()
{
    if (isRunningNow())
    {
        m_native_handler->terminate(m_pid);
    }
}

//---------------------------------------------------------------------------------------------------------------------

bool ProcessHandler::isRunning() const
{
    return m_pid != 0;
}

//---------------------------------------------------------------------------------------------------------------------

bool ProcessHandler::isRunningNow()
{
    if (isRunning())
    {
        slotCheckState();
    }

    return isRunning();
}

//---------------------------------------------------------------------------------------------------------------------

void ProcessHandler::slotCheckState()
{
    m_check_timer.stop();

    if (!validProcessExists(m_native_handler->getExecPath(m_pid), m_exec_regex))
    {
        stopMonitoring();
        emit signalProcessDied();
        return;
    }

    m_check_timer.start();
}
}  // namespace os
