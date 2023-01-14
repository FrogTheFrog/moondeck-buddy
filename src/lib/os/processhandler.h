#pragma once

// system/Qt includes
#include <QTimer>
#include <QRegularExpression>

// local includes
#include "nativeprocesshandlerinterface.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class ProcessHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ProcessHandler)

public:
    explicit ProcessHandler(std::unique_ptr<NativeProcessHandlerInterface> native_handler);
    ~ProcessHandler() override = default;

    bool startMonitoring(uint pid, const QRegularExpression& exec_regex);
    void stopMonitoring();

    void close(std::optional<uint> auto_termination_timer);
    void terminate();

    bool isRunning() const;
    bool isRunningNow();

signals:
    void signalProcessDied();

private slots:
    void slotCheckState();

private:
    uint               m_pid{0};
    QRegularExpression m_exec_regex;
    QTimer             m_check_timer;
    QTimer             m_kill_timer;

    std::unique_ptr<NativeProcessHandlerInterface> m_native_handler;
};
}  // namespace os
