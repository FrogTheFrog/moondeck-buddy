// header file include
#include "os/steamhandler.h"

// system/Qt includes
#include <QProcess>
#include <QThread>

// local includes
#include "os/shared/steamregistryobserverinterface.h"
#include "os/steam/steamwebhelperlogtracker.h"
#include "shared/loggingcategories.h"

namespace
{
const int MS_TO_SEC{1000};
}  // namespace

namespace os
{
SteamHandler::SteamHandler(std::function<QString()>                        steam_exec_path_getter,
                           std::unique_ptr<SteamProcessTracker>            steam_process_tracker,
                           std::unique_ptr<SteamRegistryObserverInterface> registry_observer)
    : m_steam_exec_path_getter{std::move(steam_exec_path_getter)}
    , m_steam_process_tracker{std::move(steam_process_tracker)}
    , m_registry_observer{std::move(registry_observer)}
{
    Q_ASSERT(m_steam_exec_path_getter);
    Q_ASSERT(m_steam_process_tracker);
    Q_ASSERT(m_registry_observer);

    connect(m_steam_process_tracker.get(), &SteamProcessTracker::signalProcessStateChanged, this,
            &SteamHandler::slotSteamProcessStateChanged);
    connect(m_registry_observer.get(), &SteamRegistryObserverInterface::signalGlobalAppId, this,
            &SteamHandler::slotGlobalAppId);
    connect(m_registry_observer.get(), &SteamRegistryObserverInterface::signalTrackedAppIsRunning, this,
            &SteamHandler::slotTrackedAppIsRunning);
    connect(m_registry_observer.get(), &SteamRegistryObserverInterface::signalTrackedAppIsUpdating, this,
            &SteamHandler::slotTrackedAppIsUpdating);
    connect(&m_steam_close_timer, &QTimer::timeout, this, &SteamHandler::slotTerminateSteam);

    m_steam_close_timer.setSingleShot(true);
}

SteamHandler::~SteamHandler() = default;

bool SteamHandler::isRunning() const
{
    return m_steam_process_tracker->isRunning();
}

bool SteamHandler::isRunningNow()
{
    return m_steam_process_tracker->isRunningNow();
}

bool SteamHandler::close(std::optional<uint> grace_period_in_sec)
{
    if (!m_steam_process_tracker->isRunningNow())
    {
        m_steam_close_timer.stop();
        return true;
    }

    // Try to shut down steam gracefully first
    const auto exec_path = m_steam_exec_path_getter();
    if (!exec_path.isEmpty())
    {
        const auto result = QProcess::startDetached(exec_path, {"-shutdown"});
        if (!result)
        {
            qCWarning(lc::os) << "Failed to start Steam shutdown sequence! Using others means to close steam...";
            m_steam_process_tracker->close(std::nullopt);
        }
    }
    else
    {
        qCWarning(lc::os) << "Steam EXEC path is not available yet, using other means of closing!";
        m_steam_process_tracker->close(std::nullopt);
    }

    if (grace_period_in_sec)
    {
        // Restart timer only it is not already running or the grace period has changed
        const auto time_in_msec = grace_period_in_sec.value() * MS_TO_SEC;
        if (!m_steam_close_timer.isActive() || static_cast<uint>(m_steam_close_timer.interval()) != time_in_msec)
        {
            m_steam_close_timer.start(static_cast<int>(time_in_msec));
        }
    }

    return true;
}

// NOLINTNEXTLINE(*-cognitive-complexity)
bool SteamHandler::launchApp(uint app_id, bool force_big_picture)
{
    const auto exec_path = m_steam_exec_path_getter();
    if (exec_path.isEmpty())
    {
        qCWarning(lc::os) << "Steam EXEC path is not available yet!";
        return false;
    }

    if (m_steam_close_timer.isActive())
    {
        qCWarning(lc::os) << "Already closing Steam, will not launch new app!";
        return false;
    }

    if (app_id == 0)
    {
        qCWarning(lc::os) << "Will not launch app with 0 ID!";
        return false;
    }

    if (getRunningApp() != app_id)
    {
        const bool is_steam_running{m_steam_process_tracker->isRunningNow()};
        if (force_big_picture && is_steam_running)
        {
            QProcess steam_process;
            steam_process.setStandardOutputFile(QProcess::nullDevice());
            steam_process.setStandardErrorFile(QProcess::nullDevice());
            steam_process.setProgram(exec_path);
            steam_process.setArguments({"steam://open/bigpicture"});

            if (!steam_process.startDetached())
            {
                qCWarning(lc::os) << "Failed to open Steam in big picture mode!";
                return false;
            }

            // Need to wait a little until Steam actually goes into bigpicture mode...
            const uint time_it_takes_steam_to_open_big_picture{1000};
            QThread::msleep(time_it_takes_steam_to_open_big_picture);
        }

        QProcess steam_process;
        steam_process.setStandardOutputFile(QProcess::nullDevice());
        steam_process.setStandardErrorFile(QProcess::nullDevice());
        steam_process.setProgram(exec_path);
        steam_process.setArguments((force_big_picture && !is_steam_running ? QStringList{"-bigpicture"} : QStringList{})
                                   + QStringList{"-applaunch", QString::number(app_id)});

        if (!steam_process.startDetached())
        {
            qCWarning(lc::os) << "Failed to start Steam app launch sequence!";
            return false;
        }

        m_tracked_app = TrackedAppData{app_id, false, false};
        m_registry_observer->startTrackingApp(app_id);
    }

    return true;
}

uint SteamHandler::getRunningApp() const
{
    return m_steam_process_tracker->isRunning()
               ? m_tracked_app && m_tracked_app->m_is_running ? m_tracked_app->m_app_id : m_global_app_id
               : 0;
}

std::optional<uint> SteamHandler::getTrackedActiveApp() const
{
    return m_steam_process_tracker->isRunning() && m_tracked_app
                   && (m_tracked_app->m_is_running || m_tracked_app->m_is_updating)
               ? std::make_optional(m_tracked_app->m_app_id)
               : std::nullopt;
}

std::optional<uint> SteamHandler::getTrackedUpdatingApp() const
{
    return m_steam_process_tracker->isRunning() && m_tracked_app && m_tracked_app->m_is_updating
               ? std::make_optional(m_tracked_app->m_app_id)
               : std::nullopt;
}

void SteamHandler::clearTrackedApp()
{
    m_registry_observer->stopTrackingApp();
    if (m_tracked_app)
    {
        m_tracked_app = std::nullopt;
    }
}

void SteamHandler::slotSteamProcessStateChanged()
{
    const bool currently_running{m_steam_process_tracker->isRunning()};
    if (currently_running)
    {
        const auto& data{m_steam_process_tracker->getProcessData()};
        qCInfo(lc::os) << "Steam is running! PID:" << data.m_pid << "START_TIME:" << data.m_start_time;
        m_registry_observer->startAppObservation();

        m_log_trackers = {std::make_unique<SteamWebHelperLogTracker>(data.m_log_dir, data.m_start_time)};
    }
    else
    {
        qCInfo(lc::os) << "Steam is no longer running!";

        m_registry_observer->stopAppObservation();
        clearTrackedApp();

        m_steam_close_timer.stop();
        m_global_app_id = 0;

        m_log_trackers = {};
    }

    emit signalProcessStateChanged();
}

void SteamHandler::slotGlobalAppId(uint app_id)
{
    if (app_id != m_global_app_id)
    {
        m_global_app_id = app_id;
        qCDebug(lc::os) << "Running appID change detected (via global key):" << m_global_app_id;
    }
}

void SteamHandler::slotTrackedAppIsRunning(bool state)
{
    if (!m_tracked_app)
    {
        qCDebug(lc::os) << "Received update for tracked app that is no longer tracked";
        return;
    }

    qCDebug(lc::os).nospace() << "App " << m_tracked_app->m_app_id << " \"running\" value change detected: " << state;
    m_tracked_app->m_is_running = state;
}

void SteamHandler::slotTrackedAppIsUpdating(bool state)
{
    if (!m_tracked_app)
    {
        qCDebug(lc::os) << "Received update for tracked app that is no longer tracked";
        return;
    }

    qCDebug(lc::os).nospace() << "App " << m_tracked_app->m_app_id << " \"updating\" value change detected: " << state;
    m_tracked_app->m_is_updating = state;
}

void SteamHandler::slotTerminateSteam()
{
    qCWarning(lc::os) << "Forcefully killing Steam...";

    const uint time_to_kill{10000};
    m_steam_process_tracker->close(time_to_kill);
}
}  // namespace os
