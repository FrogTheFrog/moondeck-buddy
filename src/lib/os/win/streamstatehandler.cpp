// header file include
#include "streamstatehandler.h"

// local includes
#include "shared/constants.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
StreamStateHandler::StreamStateHandler(std::shared_ptr<ProcessEnumerator>& enumerator)
    : m_helper_process{QRegularExpression{R"([\\/])" + shared::APP_NAME_STREAM + R"(\.exe$)",
                                          QRegularExpression::CaseInsensitiveOption},
                       enumerator}
    , m_streamer_process{QRegularExpression{R"([\\/]nvstreamer\.exe$)", QRegularExpression::CaseInsensitiveOption},
                         enumerator}
{
    connect(&m_helper_process, &os::ProcessTracker::signalProcessStateChanged, this,
            &StreamStateHandler::slotHandleProcessStateChanges);
    connect(&m_streamer_process, &os::ProcessTracker::signalProcessStateChanged, this,
            &StreamStateHandler::slotHandleProcessStateChanges);
}

//---------------------------------------------------------------------------------------------------------------------

void StreamStateHandler::endStream()
{
    if (m_state == shared::StreamState::Streaming)
    {
        m_helper_process.close();
        m_streamer_process.close();

        m_state = shared::StreamState::StreamEnding;
        emit signalStreamStateChanged();
    }
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
            if (m_helper_process.isRunning() && m_streamer_process.isRunning())
            {
                m_state = shared::StreamState::Streaming;
                emit signalStreamStateChanged();
            }
            break;
        }
        case shared::StreamState::Streaming:
        {
            if (!m_helper_process.isRunning() || !m_streamer_process.isRunning())
            {
                endStream();
            }
            break;
        }
        case shared::StreamState::StreamEnding:
        {
            if (!m_helper_process.isRunning() && !m_streamer_process.isRunning())
            {
                m_state = shared::StreamState::NotStreaming;
                emit signalStreamStateChanged();
            }
            break;
        }
    }
}
}  // namespace os
