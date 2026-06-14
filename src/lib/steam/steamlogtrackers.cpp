// header file include
#include "steam/steamlogtrackers.h"

// local includes
#include "steam/steamlogtracker.h"

namespace steam
{

SteamLogTrackers::SteamLogTrackers(const std::filesystem::path& logs_dir, const QDateTime& start_time)
    : m_web_helper_log{logs_dir, start_time}
    , m_content_log{logs_dir, start_time}
    , m_game_process_log{logs_dir, start_time}
    , m_shader_log{logs_dir, start_time}
    , m_connection_log{logs_dir, start_time}
{
    connect(&m_web_helper_log, &SteamLogTracker::signalStateChanged, this, &SteamLogTrackers::slotOnTrackerChanged);
    connect(&m_content_log, &SteamLogTracker::signalStateChanged, this, &SteamLogTrackers::slotOnTrackerChanged);
    connect(&m_game_process_log, &SteamLogTracker::signalStateChanged, this, &SteamLogTrackers::slotOnTrackerChanged);
    connect(&m_shader_log, &SteamLogTracker::signalStateChanged, this, &SteamLogTrackers::slotOnTrackerChanged);
    connect(&m_connection_log, &SteamLogTracker::signalStateChanged, this, &SteamLogTrackers::slotOnTrackerChanged);

    connect(&m_read_timer, &QTimer::timeout, this, &SteamLogTrackers::slotCheckLogs);
    m_read_timer.setSingleShot(true);
    m_read_timer.setInterval(500);
    slotCheckLogs();
}

const SteamWebHelperLogTracker& SteamLogTrackers::getWebHelperLog() const
{
    return m_web_helper_log;
}

const SteamContentLogTracker& SteamLogTrackers::getContentLog() const
{
    return m_content_log;
}

const SteamGameProcessLogTracker& SteamLogTrackers::getGameProcessLog() const
{
    return m_game_process_log;
}

const SteamShaderLogTracker& SteamLogTrackers::getShaderLog() const
{
    return m_shader_log;
}

const SteamConnectionLogTracker& SteamLogTrackers::getConnectionLog() const
{
    return m_connection_log;
}

void SteamLogTrackers::slotCheckLogs()
{
    m_read_timer.stop();
    const auto auto_start_timer{qScopeGuard([this]() { m_read_timer.start(); })};

    m_web_helper_log.slotCheckLog();
    m_content_log.slotCheckLog();
    m_game_process_log.slotCheckLog();
    m_shader_log.slotCheckLog();
    m_connection_log.slotCheckLog();
}

void SteamLogTrackers::slotOnTrackerChanged()
{
    if (!m_pending)
    {
        m_pending = true;
        QTimer::singleShot(0, this,
                           [this]()
                           {
                               m_pending = false;
                               emit signalStateChanged();
                           });
    }
}
}  // namespace steam
