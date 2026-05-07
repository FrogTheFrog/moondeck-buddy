#pragma once

// local includes
#include "common/enums.h"
#include "utils/heartbeat.h"

class StreamStateHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(StreamStateHandler)

public:
    explicit StreamStateHandler(const QString& heartbeat_key);
    ~StreamStateHandler() override = default;

    bool               endStream();
    enums::StreamState getCurrentState() const;

signals:
    void signalStreamStateChanged();

private slots:
    void slotHandleProcessStateChanges();

private:
    enums::StreamState m_state{enums::StreamState::NotStreaming};
    utils::Heartbeat   m_helper_heartbeat;
};
