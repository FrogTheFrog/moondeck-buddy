#pragma once

// A SEPARATE WINDOWS INCLUDE BECAUSE OF THE SMELL!
#include <windows.h>

// system/Qt includes
#include <QObject>
#include <QRegularExpression>
#include <QTimer>

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class ProcessTracker : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ProcessTracker)

public:
    explicit ProcessTracker(QRegularExpression name_regex);
    ~ProcessTracker() override = default;

    void startObserving();

    bool isRunning() const;
    bool isRunningNow();

    void close();
    void terminate();

signals:
    void signalProcessStateChanged(bool is_running);

private:
    void slotEnumerateProcesses();

    QRegularExpression m_name_regex;
    QTimer             m_update_timer;
    DWORD              m_pid{0};
};
}  // namespace os
