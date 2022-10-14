#pragma once

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

    bool isRunning() const;
    bool isRunningNow();

    void terminateAll();

private:
    void slotEnumerateProcesses();

    QRegularExpression m_name_regex;
    QTimer             m_update_timer;
    bool               m_is_running{false};
};
}  // namespace os
