// header file include
#include "pccontrolimpl.h"

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
PcControlImpl::PcControlImpl()
    : m_enumerator{std::make_shared<ProcessEnumerator>()}
    , m_steam_handler{m_enumerator}
{
    assert(m_enumerator != nullptr);

    connect(&m_steam_handler, &SteamHandler::signalSteamStateChanged, this, &PcControlImpl::slotHandleSteamStateChange);
    connect(&m_stream_state_handler, &StreamStateHandler::signalStreamStateChanged, this,
            &PcControlImpl::slotHandleStreamStateChange);

    const auto default_enumeration_interval{1000};
    m_enumerator->start(default_enumeration_interval);
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControlImpl::launchSteamApp(uint app_id)
{
    QStringList steam_args;
    if (m_stream_state_handler.getCurrentState() == shared::StreamState::Streaming || !m_steam_handler.isRunningNow())
    {
        steam_args += "-bigpicture";
    }
    return m_steam_handler.launchApp(app_id, steam_args);
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControlImpl::closeSteam(std::optional<uint> grace_period_in_sec)
{
    return m_steam_handler.close(grace_period_in_sec);
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControlImpl::shutdownPC(uint grace_period_in_sec)
{
    if (m_pc_state_handler.shutdownPC(grace_period_in_sec))
    {
        closeSteam(std::nullopt);
        return true;
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControlImpl::restartPC(uint grace_period_in_sec)
{
    if (m_pc_state_handler.restartPC(grace_period_in_sec))
    {
        closeSteam(std::nullopt);
        return true;
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------

uint PcControlImpl::getRunningApp() const
{
    return m_steam_handler.getRunningApp();
}

//---------------------------------------------------------------------------------------------------------------------

std::optional<uint> PcControlImpl::getTrackedUpdatingApp() const
{
    return m_steam_handler.getTrackedUpdatingApp();
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControlImpl::isSteamRunning() const
{
    return m_steam_handler.isRunning();
}

//---------------------------------------------------------------------------------------------------------------------

shared::StreamState PcControlImpl::getStreamState() const
{
    return m_stream_state_handler.getCurrentState();
}

//---------------------------------------------------------------------------------------------------------------------

shared::PcState PcControlImpl::getPcState() const
{
    return m_pc_state_handler.getState();
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::setAutoStart(bool enable)
{
    m_auto_start_handler.setAutoStart(enable);
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControlImpl::isAutoStartEnabled() const
{
    return m_auto_start_handler.isAutoStartEnabled();
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControlImpl::changeResolution(uint width, uint height, bool immediate)
{
    if (immediate)
    {
        return m_resolution_handler.changeResolution(width, height);
    }

    if (!m_steam_handler.isRunningNow() || getRunningApp() == 0)
    {
        m_resolution_handler.setPendingResolution(width, height);
        return true;
    }

    qCDebug(lc::os) << "Non-immediate change is discarded.";
    return false;
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::abortPendingResolutionChange()
{
    m_resolution_handler.clearPendingResolution();
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::restoreChangedResolution()
{
    m_resolution_handler.restoreResolution();
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::slotHandleSteamStateChange()
{
    if (m_steam_handler.isRunning())
    {
        qCDebug(lc::os) << "Handling Steam start.";
        m_resolution_handler.clearPendingResolution();
    }
    else
    {
        qCDebug(lc::os) << "Handling Steam exit.";
        m_stream_state_handler.endStream();
        m_resolution_handler.restoreResolution();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::slotHandleStreamStateChange()
{
    switch (m_stream_state_handler.getCurrentState())
    {
        case shared::StreamState::NotStreaming:
        {
            qCDebug(lc::os) << "Stream has ended.";
            break;
        }
        case shared::StreamState::Streaming:
        {
            qCDebug(lc::os) << "Stream started.";
            m_resolution_handler.applyPendingChange();
            break;
        }
        case shared::StreamState::StreamEnding:
        {
            qCDebug(lc::os) << "Stream is ending.";
            m_steam_handler.close(std::nullopt);
            m_resolution_handler.restoreResolution();
            break;
        }
    }
}
}  // namespace os
