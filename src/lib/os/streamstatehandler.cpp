// header file include
#include "os/streamstatehandler.h"

namespace os
{
StreamStateHandler::StreamStateHandler(const QString& heartbeat_key)
    : m_helper_heartbeat{heartbeat_key}
{
    connect(&m_helper_heartbeat, &utils::Heartbeat::signalStateChanged, this,
            &StreamStateHandler::slotHandleProcessStateChanges);
    m_helper_heartbeat.startListening();
}

bool StreamStateHandler::endStream()
{
    if (m_state == enums::StreamState::Streaming)
    {
        m_helper_heartbeat.terminate();
        m_state = enums::StreamState::StreamEnding;
        emit signalStreamStateChanged();
    }

    return true;
}

enums::StreamState StreamStateHandler::getCurrentState() const
{
    return m_state;
}

void StreamStateHandler::slotHandleProcessStateChanges()
{
    switch (m_state)
    {
        case enums::StreamState::NotStreaming:
        {
            if (m_helper_heartbeat.isAlive())
            {
                m_state = enums::StreamState::Streaming;
                emit signalStreamStateChanged();
            }
            break;
        }
        case enums::StreamState::Streaming:
        case enums::StreamState::StreamEnding:
        {
            if (!m_helper_heartbeat.isAlive())
            {
                if (m_state == enums::StreamState::Streaming)
                {
                    m_state = enums::StreamState::StreamEnding;
                    emit signalStreamStateChanged();
                }

                m_state = enums::StreamState::NotStreaming;
                emit signalStreamStateChanged();
            }
            break;
        }
    }
}
}  // namespace os
