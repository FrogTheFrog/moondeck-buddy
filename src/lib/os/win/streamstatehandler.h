#pragma once

// system/Qt includes
#include <QObject>

// local includes
#include "processtracker.h"
#include "shared/enums.h"

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
    ProcessTracker      m_helper_process;
    ProcessTracker      m_streamer_process;
};
}  // namespace os
