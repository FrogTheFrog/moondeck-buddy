#pragma once

// system/Qt includes
#include <QObject>

// local includes
#include "../streamstatehandlerinterface.h"
#include "shared/enums.h"
#include "utils/heartbeat.h"

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

    void                endStream() override;
    shared::StreamState getCurrentState() const override;

private slots:
    void slotHandleProcessStateChanges();

private:
    shared::StreamState m_state{shared::StreamState::NotStreaming};
    utils::Heartbeat    m_helper_heartbeat;
};
}  // namespace os
