#pragma once

// system/Qt includes
#include <filesystem>

// local includes
#include "os/processhandler.h"
#include "steamlogtrackers.h"

namespace steam
{
class SteamProcessTracker : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SteamProcessTracker)

public:
    explicit SteamProcessTracker();
    ~SteamProcessTracker() override;

    void close();

    bool                    isRunning() const;
    uint                    getPid() const;
    QDateTime               getStartTime() const;
    const SteamLogTrackers* getSteamLogTrackers() const;
    std::filesystem::path   getSteamDir() const;

signals:
    void signalProcessStateChanged();

public slots:
    void slotCheckState();

private:
    struct ProcessData
    {
        uint                              m_pid{0};
        QDateTime                         m_start_time;
        std::unique_ptr<SteamLogTrackers> m_log_trackers;
        std::filesystem::path             m_steam_dir;
    };

    ProcessData m_data;
    QTimer      m_check_timer;

    os::ProcessHandler m_process_handler;
};
}  // namespace steam
