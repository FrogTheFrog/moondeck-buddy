// header file include
#include "pccontrol.h"

// local includes
#if defined(Q_OS_WIN)
    #include "win/pccontrolimpl.h"
#else
    #error OS is not supported!
#endif

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
PcControl::PcControl(QString app_name)
    : m_impl{std::make_unique<PcControlImpl>(std::move(app_name))}
{
    connect(m_impl.get(), &PcControlInterface::signalPcStateChanged, this, &PcControlInterface::signalPcStateChanged);
}

//---------------------------------------------------------------------------------------------------------------------

void PcControl::launchSteamApp(uint app_id)
{
    m_impl->launchSteamApp(app_id);
}

//---------------------------------------------------------------------------------------------------------------------

void PcControl::exitSteam(std::optional<uint> grace_period_in_sec)
{
    m_impl->exitSteam(grace_period_in_sec);
}

//---------------------------------------------------------------------------------------------------------------------

void PcControl::shutdownPC(uint grace_period_in_sec)
{
    m_impl->shutdownPC(grace_period_in_sec);
}

//---------------------------------------------------------------------------------------------------------------------

void PcControl::restartPC(uint grace_period_in_sec)
{
    m_impl->restartPC(grace_period_in_sec);
}

//---------------------------------------------------------------------------------------------------------------------

uint PcControl::getRunningApp() const
{
    return m_impl->getRunningApp();
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControl::isSteamRunning() const
{
    return m_impl->isSteamRunning();
}

//---------------------------------------------------------------------------------------------------------------------

void PcControl::setAutoStart(bool enable)
{
    m_impl->setAutoStart(enable);
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControl::isAutoStartEnabled() const
{
    return m_impl->isAutoStartEnabled();
}
}  // namespace os
