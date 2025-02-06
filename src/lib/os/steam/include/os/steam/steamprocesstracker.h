#pragma once

// system/Qt includes
#include <QDateTime>
#include <QTimer>
#include <filesystem>

// forward declarations
namespace os
{
class NativeProcessHandlerInterface;
}  // namespace os

namespace os
{
class SteamProcessTracker : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SteamProcessTracker)

public:
    struct ProcessData
    {
        uint                  m_pid{0};
        QDateTime             m_start_time;
        std::filesystem::path m_log_dir;
    };

    explicit SteamProcessTracker(std::unique_ptr<NativeProcessHandlerInterface> native_handler);
    ~SteamProcessTracker() override;

    void close(const std::optional<uint>& auto_termination_timer);
    void terminate();

    bool               isRunning() const;
    bool               isRunningNow();
    const ProcessData& getProcessData() const;

signals:
    void signalProcessStateChanged();

private slots:
    void slotCheckState();

private:
    ProcessData m_data;
    QTimer      m_check_timer;
    QTimer      m_kill_timer;

    std::unique_ptr<NativeProcessHandlerInterface> m_native_handler;
};
}  // namespace os
