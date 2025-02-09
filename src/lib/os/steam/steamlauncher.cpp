// header file include
#include "os/steam/steamlauncher.h"

// system/Qt includes
#include <QProcess>

// local includes
#include "os/steam/steamprocesstracker.h"
#include "shared/loggingcategories.h"

namespace os
{
namespace
{
auto getCurrentUiMode(const SteamProcessTracker& process_tracker)
{
    if (const auto* log_trackers{process_tracker.getLogTrackers()})
    {
        return log_trackers->m_web_helper.getUiMode();
    }

    return SteamWebHelperLogTracker::UiMode::Unknown;
}
}  // namespace

SteamLauncher::SteamLauncher(const SteamProcessTracker& process_tracker, QString steam_exec,
                             const bool force_big_picture)
    : m_process_tracker{process_tracker}
    , m_steam_exec{std::move(steam_exec)}
    , m_force_big_picture{force_big_picture}
{
    QTimer::singleShot(0, this, &SteamLauncher::slotExecuteLaunch);
}

bool SteamLauncher::isSteamReady(const SteamProcessTracker& process_tracker, const bool force_big_picture)
{
    const auto ui_mode{getCurrentUiMode(process_tracker)};
    return force_big_picture ? ui_mode == SteamWebHelperLogTracker::UiMode::BigPicture
                             : ui_mode == SteamWebHelperLogTracker::UiMode::Desktop;
}

void SteamLauncher::setAppId(const uint app_id)
{
    m_app_id = app_id;
}

void SteamLauncher::slotExecuteLaunch()
{
    if (m_stage == Stage::Initial)
    {
        if (!m_process_tracker.isRunning()
            || (m_force_big_picture
                && getCurrentUiMode(m_process_tracker) != SteamWebHelperLogTracker::UiMode::BigPicture))
        {
            if (!QProcess::startDetached(m_steam_exec,
                                         m_force_big_picture ? QStringList{"steam://open/bigpicture"} : QStringList{}))
            {
                qCWarning(lc::os) << "Failed to launch Steam!";
                emit signalFinished(m_steam_exec, m_app_id, false);
                return;
            }
        }

        m_stage = Stage::WaitingForSteam;
    }

    if (m_stage == Stage::WaitingForSteam)
    {
        if (!isSteamReady(m_process_tracker, m_force_big_picture))
        {
            if (m_wait_counter < 120 /* 1 minute, we are checking this every 500ms */)
            {
                ++m_wait_counter;
                QTimer::singleShot(500, this, &SteamLauncher::slotExecuteLaunch);
                return;
            }

            qCWarning(lc::os) << "Steam did not finish loading Desktop/BigPicture mode in time!";
            emit signalFinished(m_steam_exec, m_app_id, false);
            return;
        }
    }

    emit signalFinished(m_steam_exec, m_app_id, true);
}
}  // namespace os
