// header file include
#include "os/pcstatehandler.h"

// system/Qt includes
#include <QTimer>

// os-specific includes
#if defined(Q_OS_WIN)
    #include "os/win/nativepcstatehandler.h"
#elif defined(Q_OS_LINUX)
    #include "os/linux/nativepcstatehandler.h"
#else
    #error OS is not supported!
#endif

// local includes
#include "common/loggingcategories.h"
#include "os/common/nativepcstatehandlerinterface.h"

namespace
{
const int SEC_TO_MS{1000};

int getTimeoutTime(uint grace_period_in_sec)
{
    return static_cast<int>(grace_period_in_sec) * SEC_TO_MS;
}
}  // namespace

namespace os
{
PcStateHandler::PcStateHandler()
    : m_native_handler{std::make_unique<NativePcStateHandler>()}
{
}

PcStateHandler::~PcStateHandler() = default;

enums::PcState PcStateHandler::getState() const
{
    return m_state;
}

bool PcStateHandler::shutdownPC(uint grace_period_in_sec)
{
    return doChangeState(grace_period_in_sec, "shut down", "shutdown", &NativePcStateHandlerInterface::canShutdownPC,
                         &NativePcStateHandlerInterface::shutdownPC, enums::PcState::ShuttingDown);
}

bool PcStateHandler::restartPC(uint grace_period_in_sec)
{
    return doChangeState(grace_period_in_sec, "restarted", "restart", &NativePcStateHandlerInterface::canRestartPC,
                         &NativePcStateHandlerInterface::restartPC, enums::PcState::Restarting);
}

bool PcStateHandler::suspendPC(uint grace_period_in_sec)
{
    return doChangeState(grace_period_in_sec, "suspended", "suspend", &NativePcStateHandlerInterface::canSuspendPC,
                         &NativePcStateHandlerInterface::suspendPC, enums::PcState::Suspending);
}

bool PcStateHandler::hibernatePC(uint grace_period_in_sec)
{
    return doChangeState(grace_period_in_sec, "hibernated", "hibernate", &NativePcStateHandlerInterface::canHibernatePC,
                         &NativePcStateHandlerInterface::hibernatePC, enums::PcState::Suspending);
}

bool PcStateHandler::doChangeState(uint grace_period_in_sec, const QString& cant_do_entry,
                                   // NOLINTNEXTLINE(*-swappable-parameters)
                                   const QString& failed_to_do_entry, NativeMethod can_do_method,
                                   NativeMethod do_method, enums::PcState new_state)
{
    if (m_state != enums::PcState::Normal)
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
                           qCInfo(lc::os) << "Setting PC state to transient.";
                           m_state = enums::PcState::Transient;

                           constexpr int state_reset_time{5};
                           QTimer::singleShot(getTimeoutTime(state_reset_time), this,
                                              [this]()
                                              {
                                                  qCInfo(lc::os) << "Resetting PC state back to normal.";
                                                  m_state = enums::PcState::Normal;
                                              });

                           if (!(m_native_handler.get()->*do_method)())
                           {
                               qCWarning(lc::os).nospace() << "Failed to " << failed_to_do_entry << " PC!";
                               m_state = enums::PcState::Normal;
                           }
                       });

    m_state = new_state;
    return true;
}
}  // namespace os
