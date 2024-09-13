#pragma once

// system/Qt includes
#include <QSharedMemory>
#include <QTimer>

namespace utils
{
class Heartbeat final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Heartbeat)

public:
    explicit Heartbeat(const QString& key);
    ~Heartbeat() override = default;

    void startBeating();
    void startListening();

    void terminate();
    bool isAlive() const;

signals:
    void signalShouldTerminate();
    void signalStateChanged();

private slots:
    void slotBeating(bool fresh_start);
    void slotListening();

private:
    QSharedMemory m_shared_mem;
    QTimer        m_timer;
    bool          m_is_beating{false};
    bool          m_is_listening{false};
    bool          m_is_alive{false};
};
}  // namespace utils
