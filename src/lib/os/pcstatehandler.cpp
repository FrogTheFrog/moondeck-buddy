// header file include
#include "os/pcstatehandler.h"

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

namespace os
{
PcStateHandler::PcStateHandler()
    : m_native_handler{std::make_unique<NativePcStateHandler>()}
{
    m_grace_timer.setSingleShot(true);
    connect(&m_grace_timer, &QTimer::timeout, this,
            [this]()
            {
                if (const auto action{std::exchange(m_pending_change, {})})
                {
                    action();
                }
            });

    connect(m_native_handler.get(), &NativePcStateHandler::signalWokeUp, this,
            [this]()
            {
                if (m_state == enums::PcState::Transient)
                {
                    qCInfo(lc::os) << "PC woke up.";
                    m_state = enums::PcState::Normal;
                }
            });
}

PcStateHandler::~PcStateHandler() = default;

enums::PcState PcStateHandler::getState() const
{
    return m_state;
}

bool PcStateHandler::shutdownPC(const uint grace_period_in_sec)
{
    return doChangeState(grace_period_in_sec, "shut down", "shutdown", &NativePcStateHandlerInterface::canShutdownPC,
                         &NativePcStateHandlerInterface::shutdownPC, enums::PcState::ShuttingDown);
}

bool PcStateHandler::restartPC(const uint grace_period_in_sec)
{
    return doChangeState(grace_period_in_sec, "restarted", "restart", &NativePcStateHandlerInterface::canRestartPC,
                         &NativePcStateHandlerInterface::restartPC, enums::PcState::Restarting);
}

bool PcStateHandler::suspendPC(const uint grace_period_in_sec)
{
    return doChangeState(grace_period_in_sec, "suspended", "suspend", &NativePcStateHandlerInterface::canSuspendPC,
                         &NativePcStateHandlerInterface::suspendPC, enums::PcState::Suspending);
}

bool PcStateHandler::hibernatePC(const uint grace_period_in_sec)
{
    return doChangeState(grace_period_in_sec, "hibernated", "hibernate", &NativePcStateHandlerInterface::canHibernatePC,
                         &NativePcStateHandlerInterface::hibernatePC, enums::PcState::Suspending);
}

bool PcStateHandler::abortPcStateChange()
{
    m_grace_timer.stop();

    m_pending_change = {};
    m_state          = enums::PcState::Normal;

    qCInfo(lc::os) << "State change aborted.";
    return true;
}

bool PcStateHandler::doChangeState(const uint grace_period_in_sec, const QString& cant_do_entry,
                                   // NOLINTNEXTLINE(*-swappable-parameters)
                                   const QString& failed_to_do_entry, const NativeMethod can_do_method,
                                   const NativeMethod do_method, const enums::PcState new_state)
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

    m_pending_change = [this, failed_to_do_entry, do_method]()
    {
        qCInfo(lc::os) << "Setting PC state to transient.";
        m_state = enums::PcState::Transient;

        qCInfo(lc::os).nospace() << "Trying to " << failed_to_do_entry << " PC.";
        if (!(m_native_handler.get()->*do_method)())
        {
            qCWarning(lc::os).nospace() << "Failed to " << failed_to_do_entry << " PC!";
            m_state = enums::PcState::Normal;
        }
    };
    m_grace_timer.start(grace_period_in_sec * 1000);

    m_state = new_state;
    return true;
}
}  // namespace os
