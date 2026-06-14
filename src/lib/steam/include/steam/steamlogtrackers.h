#pragma once

// local includes
#include "steamconnectionlogtracker.h"
#include "steamcontentlogtracker.h"
#include "steamgameprocesslogtracker.h"
#include "steamshaderlogtracker.h"
#include "steamwebhelperlogtracker.h"

namespace steam
{
class SteamLogTrackers : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SteamLogTrackers)

public:
    explicit SteamLogTrackers(const std::filesystem::path& logs_dir, const QDateTime& start_time);
    ~SteamLogTrackers() override = default;

    const SteamWebHelperLogTracker&   getWebHelperLog() const;
    const SteamContentLogTracker&     getContentLog() const;
    const SteamGameProcessLogTracker& getGameProcessLog() const;
    const SteamShaderLogTracker&      getShaderLog() const;
    const SteamConnectionLogTracker&  getConnectionLog() const;

signals:
    void signalStateChanged();

private slots:
    void slotCheckLogs();
    void slotOnTrackerChanged();

private:
    QTimer                     m_read_timer;
    SteamWebHelperLogTracker   m_web_helper_log;
    SteamContentLogTracker     m_content_log;
    SteamGameProcessLogTracker m_game_process_log;
    SteamShaderLogTracker      m_shader_log;
    SteamConnectionLogTracker  m_connection_log;
    bool                       m_pending{false};
};
}  // namespace steam
