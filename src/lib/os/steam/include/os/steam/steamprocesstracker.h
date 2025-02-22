#pragma once

// system/Qt includes
#include <filesystem>

// local includes
#include "os/steam/steamcontentlogtracker.h"
#include "os/steam/steamgameprocesslogtracker.h"
#include "os/steam/steamshaderlogtracker.h"
#include "os/steam/steamwebhelperlogtracker.h"

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
    struct LogTrackers
    {
        QTimer                     m_read_timer;
        SteamWebHelperLogTracker   m_web_helper;
        SteamContentLogTracker     m_content_log;
        SteamGameProcessLogTracker m_gameprocess_log;
        SteamShaderLogTracker      m_shader_log;
    };

    explicit SteamProcessTracker(std::unique_ptr<NativeProcessHandlerInterface> native_handler);
    ~SteamProcessTracker() override;

    void close();
    void terminate();

    bool               isRunning() const;
    uint               getPid() const;
    QDateTime          getStartTime() const;
    const LogTrackers* getLogTrackers() const;

signals:
    void signalProcessStateChanged();

public slots:
    void slotCheckState();

private:
    struct ProcessData
    {
        uint                         m_pid{0};
        QDateTime                    m_start_time;
        std::filesystem::path        m_log_dir;
        std::unique_ptr<LogTrackers> m_log_trackers;
    };

    ProcessData m_data;
    QTimer      m_check_timer;

    std::unique_ptr<NativeProcessHandlerInterface> m_native_handler;
};
}  // namespace os
