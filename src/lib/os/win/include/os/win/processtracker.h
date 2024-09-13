#pragma once

// system/Qt includes
#include <QRegularExpression>
#include <memory>

// local includes
#include "os/win/processenumerator.h"

namespace os
{
// DEPRECATED, to be removed once Nvidia is gone
class ProcessTracker : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ProcessTracker)

public:
    explicit ProcessTracker(QRegularExpression name_regex, std::shared_ptr<ProcessEnumerator> enumerator);
    ~ProcessTracker() override = default;

    bool isRunning() const;
    bool isRunningNow();

    void close();
    void terminate();

signals:
    void signalProcessStateChanged();

private slots:
    void slotUpdateProcessState(const std::vector<ProcessEnumerator::ProcessData>& data);

private:
    QRegularExpression                 m_name_regex;
    std::shared_ptr<ProcessEnumerator> m_enumerator;
    DWORD                              m_pid{0};
};
}  // namespace os
