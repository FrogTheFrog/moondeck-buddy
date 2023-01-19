#pragma once

// system/Qt includes
#include <QObject>

// local includes
#include "../streamstatehandlerinterface.h"
#include "processtracker.h"
#include "shared/enums.h"
#include "utils/heartbeat.h"

//---------------------------------------------------------------------------------------------------------------------

// TODO: remove
// This is a temp. hack for Nvidia until it kills GameStream
bool& getMouseAccelResetHack();

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class StreamStateHandler : public StreamStateHandlerInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(StreamStateHandler)

public:
    explicit StreamStateHandler();
    ~StreamStateHandler() override = default;

    bool               endStream() override;
    enums::StreamState getCurrentState() const override;

private slots:
    void slotHandleProcessStateChanges();

private:
    enums::StreamState m_state{enums::StreamState::NotStreaming};
    utils::Heartbeat    m_helper_heartbeat;
    ProcessTracker      m_nvidia_stream_process;
};
}  // namespace os
