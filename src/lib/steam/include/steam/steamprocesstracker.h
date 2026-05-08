#pragma once

// system/Qt includes
#include <filesystem>

// local includes
#include "os/processhandler.h"
#include "steamconnectionlogtracker.h"
#include "steamcontentlogtracker.h"
#include "steamgameprocesslogtracker.h"
#include "steamshaderlogtracker.h"
#include "steamwebhelperlogtracker.h"

namespace steam
{
class SteamProcessTracker : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SteamProcessTracker)

public:
    struct LogTrackers
    {
        QTimer                     m_read_timer;
        SteamWebHelperLogTracker   m_web_helper;
        SteamContentLogTracker     m_content_log;
        SteamGameProcessLogTracker m_gameprocess_log;
        SteamShaderLogTracker      m_shader_log;
        SteamConnectionLogTracker  m_connection_log;
    };

    explicit SteamProcessTracker();
    ~SteamProcessTracker() override;

    void close();

    bool                  isRunning() const;
    uint                  getPid() const;
    QDateTime             getStartTime() const;
    const LogTrackers*    getLogTrackers() const;
    std::filesystem::path getSteamDir() const;

signals:
    void signalProcessStateChanged();

public slots:
    void slotCheckState();

private slots:
    void slotCheckLogs();

private:
    struct ProcessData
    {
        uint                         m_pid{0};
        QDateTime                    m_start_time;
        std::unique_ptr<LogTrackers> m_log_trackers;
        std::filesystem::path        m_steam_dir;
    };

    ProcessData m_data;
    QTimer      m_check_timer;

    os::ProcessHandler m_process_handler;
};
}  // namespace steam
