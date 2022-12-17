#pragma once

// system/Qt includes
#include <QObject>

// local includes
#include "processtracker.h"
#include "shared/enums.h"
#include "utils/heartbeat.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class StreamStateHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(StreamStateHandler)

public:
    explicit StreamStateHandler();
    ~StreamStateHandler() override = default;

    void                endStream();
    shared::StreamState getCurrentState() const;

signals:
    void signalStreamStateChanged();

private slots:
    void slotHandleProcessStateChanges();

private:
    shared::StreamState m_state{shared::StreamState::NotStreaming};
    utils::Heartbeat    m_helper_heartbeat;
};
}  // namespace os
