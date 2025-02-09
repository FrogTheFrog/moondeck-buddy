// header file include
#include "os/steam/steamappwatcher.h"

// local includes
#include "os/steam/steamprocesstracker.h"
#include "shared/loggingcategories.h"

#include <QProcess>

namespace os
{
SteamAppWatcher::SteamAppWatcher(const SteamProcessTracker& process_tracker, const uint app_id)
    : m_process_tracker{process_tracker}
    , m_app_id{app_id}
{
    connect(&m_check_timer, &QTimer::timeout, this, &SteamAppWatcher::slotCheckState);

    m_check_timer.setInterval(500);
    m_check_timer.setSingleShot(true);

    qCInfo(lc::os) << "Started watching AppID:" << m_app_id;
    slotCheckState();
}

SteamAppWatcher::~SteamAppWatcher()
{
    qCInfo(lc::os) << "Stopped watching AppID:" << m_app_id;
}

enums::AppState SteamAppWatcher::getAppState(const SteamProcessTracker& process_tracker, const uint app_id)
{
    const auto* log_trackers{process_tracker.getLogTrackers()};
    if (!log_trackers)
    {
        return enums::AppState::Stopped;
    }

    auto       new_state{enums::AppState::Stopped};
    const auto content_state{log_trackers->m_content_log.getAppState(app_id)};

    if (content_state == SteamContentLogTracker::AppState::Running)
    {
        new_state = enums::AppState::Running;
    }
    else if (content_state == SteamContentLogTracker::AppState::Updating)
    {
        new_state = enums::AppState::Updating;
    }

    return new_state;
}

enums::AppState SteamAppWatcher::getAppState() const
{
    return m_current_state;
}

uint SteamAppWatcher::getAppId() const
{
    return m_app_id;
}

void SteamAppWatcher::slotCheckState()
{
    const auto auto_start_timer{qScopeGuard([this]() { m_check_timer.start(); })};

    if (const auto new_state{getAppState(m_process_tracker, m_app_id)}; new_state != m_current_state)
    {
        if (new_state == enums::AppState::Stopped && m_delay_counter < 5)
        {
            ++m_delay_counter;
            return;
        }

        qCInfo(lc::os) << "[WATCHED] New app state for AppID" << m_app_id
                       << "detected:" << lc::qEnumToString(m_current_state) << "->" << lc::qEnumToString(new_state);
        m_current_state = new_state;
        m_delay_counter = 0;
    }
}
}  // namespace os
