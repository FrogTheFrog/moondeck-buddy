// header file include
#include "streamstatehandler.h"

// local includes
#include "shared/constants.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
StreamStateHandler::StreamStateHandler()
    : m_helper_heartbeat{shared::APP_NAME_STREAM}
{
    connect(&m_helper_heartbeat, &utils::Heartbeat::signalStateChanged, this,
            &StreamStateHandler::slotHandleProcessStateChanges);
    m_helper_heartbeat.startListening();
}

//---------------------------------------------------------------------------------------------------------------------

bool StreamStateHandler::endStream()
{
    if (m_state == shared::StreamState::Streaming)
    {
        m_helper_heartbeat.terminate();
        m_state = shared::StreamState::StreamEnding;
        emit signalStreamStateChanged();
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------

shared::StreamState StreamStateHandler::getCurrentState() const
{
    return m_state;
}

//---------------------------------------------------------------------------------------------------------------------

void StreamStateHandler::slotHandleProcessStateChanges()
{
    switch (m_state)
    {
        case shared::StreamState::NotStreaming:
        {
            if (m_helper_heartbeat.isAlive())
            {
                m_state = shared::StreamState::Streaming;
                emit signalStreamStateChanged();
            }
            break;
        }
        case shared::StreamState::Streaming:
        case shared::StreamState::StreamEnding:
        {
            if (!m_helper_heartbeat.isAlive())
            {
                m_state = shared::StreamState::NotStreaming;
                emit signalStreamStateChanged();
            }
            break;
        }
    }
}
}  // namespace os
