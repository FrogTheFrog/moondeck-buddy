// header file include
#include "pccontrolimpl.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
PcControlImpl::PcControlImpl()
    : m_enumerator{std::make_shared<ProcessEnumerator>()}
    , m_steam_handler{m_enumerator}
{
    assert(m_enumerator != nullptr);

    connect(&m_pc_state_handler, &PcStateHandler::signalPcStateChanged, this, &PcControlImpl::signalPcStateChanged);
    connect(&m_steam_handler, &SteamHandler::signalSteamStateChanged, this, &PcControlImpl::slotHandleSteamStateChange);
    connect(&m_stream_state_handler, &StreamStateHandler::signalStreamStateChanged, this,
            &PcControlImpl::slotHandleStreamStateChange);

    const auto default_enumeration_interval{1000};
    m_enumerator->start(default_enumeration_interval);
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::launchSteamApp(uint app_id)
{
    QStringList steam_args;
    if (m_stream_state_handler.getCurrentState() == shared::StreamState::Streaming)
    {
        steam_args += "-bigpicture";
    }
    m_steam_handler.launchApp(app_id, steam_args);
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::exitSteam(std::optional<uint> grace_period_in_sec)
{
    m_steam_handler.close(grace_period_in_sec);
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::shutdownPC(uint grace_period_in_sec)
{
    m_pc_state_handler.shutdownPC(grace_period_in_sec);
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::restartPC(uint grace_period_in_sec)
{
    m_pc_state_handler.restartPC(grace_period_in_sec);
}

//---------------------------------------------------------------------------------------------------------------------

uint PcControlImpl::getRunningApp() const
{
    return m_steam_handler.getRunningApp();
}

//---------------------------------------------------------------------------------------------------------------------

std::optional<uint> PcControlImpl::isLastLaunchedAppUpdating() const
{
    return m_steam_handler.isLastLaunchedAppUpdating();
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

void PcControlImpl::changeResolution(uint width, uint height, bool immediate)
{
    if (immediate)
    {
        m_resolution_handler.changeResolution(width, height);
    }
    else if (!m_steam_handler.isRunningNow() || getRunningApp() == 0)
    {
        m_resolution_handler.setPendingResolution(width, height);
    }
    else
    {
        qDebug("Non-immediate change is discarded.");
    }
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
        qDebug("Handling Steam start.");
        m_resolution_handler.clearPendingResolution();
    }
    else
    {
        qDebug("Handling Steam exit.");
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
            qDebug("Stream has ended.");
            break;
        }
        case shared::StreamState::Streaming:
        {
            qDebug("Stream started.");
            m_resolution_handler.applyPendingChange();
            break;
        }
        case shared::StreamState::StreamEnding:
        {
            qDebug("Stream is ending.");
            m_steam_handler.close(std::nullopt);
            m_resolution_handler.restoreResolution();
            break;
        }
    }
}
}  // namespace os
