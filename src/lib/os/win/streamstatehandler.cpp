// header file include
#include "streamstatehandler.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
StreamStateHandler::StreamStateHandler()
    : m_helper_process{QRegularExpression{R"([\\/]MoonDeckStream\.exe$)", QRegularExpression::CaseInsensitiveOption}}
    , m_streamer_process{QRegularExpression{R"([\\/]nvstreamer\.exe$)", QRegularExpression::CaseInsensitiveOption}}
{
    connect(&m_helper_process, &os::ProcessTracker::signalProcessStateChanged, this,
            &StreamStateHandler::slotHandleProcessStateChanges);
    connect(&m_streamer_process, &os::ProcessTracker::signalProcessStateChanged, this,
            &StreamStateHandler::slotHandleProcessStateChanges);
    m_helper_process.startObserving();
    m_streamer_process.startObserving();
}

//---------------------------------------------------------------------------------------------------------------------

void StreamStateHandler::endStream()
{
    if (streaming_started)
    {
        m_helper_process.close();
        m_streamer_process.close();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void StreamStateHandler::slotHandleProcessStateChanges()
{
    if (m_helper_process.isRunning() && m_streamer_process.isRunning())
    {
        streaming_started = true;
        emit signalStreamStarted();
    }
    else if (streaming_started)
    {
        streaming_started = false;
        m_helper_process.close();
        m_streamer_process.close();
        emit signalStreamEnded();
    }
}
}  // namespace os
