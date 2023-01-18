// header file include
#include "pcstatehandler.h"

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const int EXTRA_DELAY_SECS{10};
const int SEC_TO_MS{1000};

//---------------------------------------------------------------------------------------------------------------------

int getTimeoutTime(uint grace_period_in_sec)
{
    return static_cast<int>(grace_period_in_sec) * SEC_TO_MS;
}

//---------------------------------------------------------------------------------------------------------------------

int getResetTime(uint grace_period_in_sec)
{
    return static_cast<int>(grace_period_in_sec + EXTRA_DELAY_SECS) * SEC_TO_MS;
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
PcStateHandler::PcStateHandler(std::unique_ptr<NativePcStateHandlerInterface> native_handler)
    : m_native_handler{std::move(native_handler)}
{
    assert(m_native_handler);

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
    return doChangeState(grace_period_in_sec, "shut down", "shutdown", &NativePcStateHandlerInterface::canShutdownPC,
                         &NativePcStateHandlerInterface::shutdownPC, shared::PcState::ShuttingDown);
}

//---------------------------------------------------------------------------------------------------------------------

bool PcStateHandler::restartPC(uint grace_period_in_sec)
{
    return doChangeState(grace_period_in_sec, "restarted", "restart", &NativePcStateHandlerInterface::canRestartPC,
                         &NativePcStateHandlerInterface::restartPC, shared::PcState::Restarting);
}

//---------------------------------------------------------------------------------------------------------------------

bool PcStateHandler::suspendPC(uint grace_period_in_sec)
{
    return doChangeState(grace_period_in_sec, "suspended", "suspend", &NativePcStateHandlerInterface::canSuspendPC,
                         &NativePcStateHandlerInterface::suspendPC, shared::PcState::Suspending);
}

//---------------------------------------------------------------------------------------------------------------------

bool PcStateHandler::doChangeState(uint grace_period_in_sec, const QString& cant_do_entry,
                                   // NOLINTNEXTLINE(*-swappable-parameters)
                                   const QString& failed_to_do_entry, NativeMethod can_do_method,
                                   NativeMethod do_method, shared::PcState new_state)
{
    if (m_state_change_back_timer.isActive())
    {
        qCDebug(lc::os) << "PC is already changing state. Aborting request.";
        return false;
    }

    if (!(m_native_handler.get()->*can_do_method)())
    {
        qCWarning(lc::os).nospace() << "PC cannot be " << cant_do_entry << "!";
        return false;
    }

    QTimer::singleShot(getTimeoutTime(grace_period_in_sec), this,
                       [this, failed_to_do_entry, do_method]()
                       {
                           if (!(m_native_handler.get()->*do_method)())
                           {
                               qCWarning(lc::os).nospace() << "Failed to " << failed_to_do_entry << " PC!";
                               slotResetState();
                           }
                       });

    m_state_change_back_timer.start(getResetTime(grace_period_in_sec));
    m_state = new_state;

    return true;
}

//---------------------------------------------------------------------------------------------------------------------

void PcStateHandler::slotResetState()
{
    m_state_change_back_timer.stop();
    qCInfo(lc::os) << "Failed to change the PC state - resetting back to normal.";
    m_state = shared::PcState::Normal;
}
}  // namespace os
