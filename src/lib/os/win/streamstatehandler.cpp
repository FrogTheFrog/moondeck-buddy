// header file include
#include "streamstatehandler.h"

// local includes
#include "shared/constants.h"
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

// TODO: remove hack
bool& getMouseAccelResetHack()
{
    static bool value{false};
    return value;
}

//---------------------------------------------------------------------------------------------------------------------

namespace
{
QString getError(LSTATUS status)
{
    return QString::fromStdString(std::system_category().message(status));
}

//---------------------------------------------------------------------------------------------------------------------

void SetMouseAcceleration(bool value)
{
    // NOLINTNEXTLINE(*)
    int mouseParams[3];
    // NOLINTNEXTLINE(*)
    if (SystemParametersInfoW(SPI_GETMOUSE, 0, mouseParams, 0) == FALSE)
    {
        qCWarning(lc::os) << "Failed to get system params for mouse. Reason:"
                          << getError(static_cast<LSTATUS>(GetLastError()));
    }

    mouseParams[2] = value ? 1 : 0;
    // NOLINTNEXTLINE(*)
    if (SystemParametersInfoW(SPI_SETMOUSE, 0, mouseParams, SPIF_SENDCHANGE) == FALSE)
    {
        qCWarning(lc::os) << "Failed to set system params for mouse. Reason:"
                          << getError(static_cast<LSTATUS>(GetLastError()));
    }
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
StreamStateHandler::StreamStateHandler()
    : m_helper_heartbeat{shared::APP_NAME_STREAM}  // Temporary support until nvidia completely removes the gamestream
    , m_nvidia_stream_process(
          QRegularExpression{R"([\\\/]nvstreamer\.exe$)", QRegularExpression::CaseInsensitiveOption},
          std::make_shared<ProcessEnumerator>())
{
    connect(&m_helper_heartbeat, &utils::Heartbeat::signalStateChanged, this,
            &StreamStateHandler::slotHandleProcessStateChanges);
    m_helper_heartbeat.startListening();
}

//---------------------------------------------------------------------------------------------------------------------

void StreamStateHandler::endStream()
{
    if (m_state == shared::StreamState::Streaming)
    {
        m_helper_heartbeat.terminate();
        if (m_nvidia_stream_process.isRunningNow())
        {
            m_nvidia_stream_process.close();
            if (getMouseAccelResetHack())
            {
                SetMouseAcceleration(false);
            }
        }
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
                if (m_nvidia_stream_process.isRunningNow())
                {
                    m_nvidia_stream_process.close();
                    if (getMouseAccelResetHack())
                    {
                        SetMouseAcceleration(false);
                    }
                }
                m_state = shared::StreamState::NotStreaming;
                emit signalStreamStateChanged();
            }
            break;
        }
    }
}
}  // namespace os
