#pragma once

// A SEPARATE WINDOWS INCLUDE BECAUSE OF THE SMELL!
#include <windows.h>

// system/Qt includes
#include <QObject>
#include <QTimer>

namespace os
{
// DEPRECATED, to be removed once Nvidia is gone
class ProcessEnumerator : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ProcessEnumerator)

public:
    struct ProcessData
    {
        DWORD   m_pid;
        QString m_name;
    };

    explicit ProcessEnumerator();
    ~ProcessEnumerator() override = default;

    void start(uint interval_ms);
    void stop();

public slots:
    void slotEnumerate();

signals:
    void signalProcesses(const std::vector<ProcessData>& data);

private:
    QTimer m_timer;
    bool   m_started{false};
};
}  // namespace os
