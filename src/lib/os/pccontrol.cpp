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
PcControl::PcControl()
    : m_impl{std::make_unique<PcControlImpl>()}
{
    connect(m_impl.get(), &PcControlInterface::signalShowTrayMessage, this, &PcControlInterface::signalShowTrayMessage);
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControl::launchSteamApp(uint app_id)
{
    return m_impl->launchSteamApp(app_id);
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControl::closeSteam(std::optional<uint> grace_period_in_sec)
{
    return m_impl->closeSteam(grace_period_in_sec);
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControl::shutdownPC(uint grace_period_in_sec)
{
    return m_impl->shutdownPC(grace_period_in_sec);
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControl::restartPC(uint grace_period_in_sec)
{
    return m_impl->restartPC(grace_period_in_sec);
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControl::suspendPC(uint grace_period_in_sec)
{
    return m_impl->suspendPC(grace_period_in_sec);
}

//---------------------------------------------------------------------------------------------------------------------

uint PcControl::getRunningApp() const
{
    return m_impl->getRunningApp();
}

//---------------------------------------------------------------------------------------------------------------------

std::optional<uint> PcControl::getTrackedUpdatingApp() const
{
    return m_impl->getTrackedUpdatingApp();
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControl::isSteamRunning() const
{
    return m_impl->isSteamRunning();
}

//---------------------------------------------------------------------------------------------------------------------

shared::StreamState PcControl::getStreamState() const
{
    return m_impl->getStreamState();
}

//---------------------------------------------------------------------------------------------------------------------

shared::PcState PcControl::getPcState() const
{
    return m_impl->getPcState();
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

//---------------------------------------------------------------------------------------------------------------------

bool PcControl::changeResolution(uint width, uint height, bool immediate)
{
    return m_impl->changeResolution(width, height, immediate);
}

//---------------------------------------------------------------------------------------------------------------------

void PcControl::abortPendingResolutionChange()
{
    m_impl->abortPendingResolutionChange();
}

//---------------------------------------------------------------------------------------------------------------------

void PcControl::restoreChangedResolution()
{
    return m_impl->restoreChangedResolution();
}
}  // namespace os
