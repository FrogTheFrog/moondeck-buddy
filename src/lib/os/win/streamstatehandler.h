#pragma once

// system/Qt includes
#include <QObject>

// local includes
#include "processtracker.h"

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

    void endStream();

signals:
    void signalStreamStarted();
    void signalStreamEnded();

private slots:
    void slotHandleProcessStateChanges();

private:
    bool           streaming_started{false};
    ProcessTracker m_helper_process;
    ProcessTracker m_streamer_process;
};
}  // namespace os
