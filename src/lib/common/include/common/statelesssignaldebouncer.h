#pragma once

// system/Qt includes
#include <QObject>

namespace common
{
// Debounces the signals until NEXT Qt signal loop.
class StatelessSignalDebouncer final : public QObject
{
    Q_OBJECT

public:
    explicit StatelessSignalDebouncer(int debounce_time_ms = 0);
    ~StatelessSignalDebouncer() override = default;

signals:
    void signalInput();
    void signalOutput();

private:
    bool m_pending{false};
};
}  // namespace common