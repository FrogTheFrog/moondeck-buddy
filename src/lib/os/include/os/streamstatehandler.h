#pragma once

// local includes
#include "os/streamstatehandlerinterface.h"
#include "shared/enums.h"
#include "utils/heartbeat.h"

namespace os
{
class StreamStateHandler : public StreamStateHandlerInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(StreamStateHandler)

public:
    explicit StreamStateHandler(const QString& heartbeat_key);
    ~StreamStateHandler() override = default;

    bool               endStream() override;
    enums::StreamState getCurrentState() const override;

private slots:
    void slotHandleProcessStateChanges();

private:
    enums::StreamState m_state{enums::StreamState::NotStreaming};
    utils::Heartbeat   m_helper_heartbeat;
};
}  // namespace os
