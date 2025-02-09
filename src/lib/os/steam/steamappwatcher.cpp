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
    m_check_timer.setInterval(500);
    m_check_timer.setSingleShot(true);

    slotCheckState();
}

void SteamAppWatcher::slotCheckState()
{
    const auto* log_trackers{m_process_tracker.getLogTrackers()};
    if (!log_trackers)
    {
        m_current_state = enums::AppState::Stopped;
        m_delay_counter = 0;
        return;
    }

    auto       new_state{enums::AppState::Stopped};
    const auto content_state{log_trackers->m_content_log.getAppState(m_app_id)};

    if (content_state == SteamContentLogTracker::AppState::Running)
    {
        new_state = enums::AppState::Running;
    }
    else if (content_state == SteamContentLogTracker::AppState::Updating)
    {
        new_state = enums::AppState::Updating;
    }

    if (new_state != m_current_state)
    {
        if (new_state == enums::AppState::Stopped && m_delay_counter < 5)
        {
            ++m_delay_counter;
            return;
        }

        qCInfo(lc::os) << "New watched app state for AppID" << m_app_id
                       << "detected:" << lc::qEnumToString(m_current_state) << "->" << lc::qEnumToString(new_state);
        m_current_state = new_state;
        m_delay_counter = 0;
    }
}
}  // namespace os
