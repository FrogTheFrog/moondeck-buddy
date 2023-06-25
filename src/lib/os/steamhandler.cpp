// header file include
#include "steamhandler.h"

// system/Qt includes
#include <QProcess>
#include <QThread>

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QRegularExpression STEAM_EXEC_PATTERN{R"([\\\/]steam(?:\.exe$|$))", QRegularExpression::CaseInsensitiveOption};
const int                MS_TO_SEC{1000};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
SteamHandler::SteamHandler(std::unique_ptr<ProcessHandler>                 process_handler,
                           std::unique_ptr<SteamRegistryObserverInterface> registry_observer)
    : m_process_handler{std::move(process_handler)}
    , m_registry_observer{std::move(registry_observer)}
{
    Q_ASSERT(m_process_handler != nullptr);
    Q_ASSERT(m_registry_observer != nullptr);

    connect(m_process_handler.get(), &ProcessHandler::signalProcessDied, this, &SteamHandler::slotSteamProcessDied);
    connect(m_registry_observer.get(), &SteamRegistryObserverInterface::signalSteamExecPath, this,
            &SteamHandler::slotSteamExecPath);
    connect(m_registry_observer.get(), &SteamRegistryObserverInterface::signalSteamPID, this,
            &SteamHandler::slotSteamPID);
    connect(m_registry_observer.get(), &SteamRegistryObserverInterface::signalGlobalAppId, this,
            &SteamHandler::slotGlobalAppId);
    connect(m_registry_observer.get(), &SteamRegistryObserverInterface::signalTrackedAppIsRunning, this,
            &SteamHandler::slotTrackedAppIsRunning);
    connect(m_registry_observer.get(), &SteamRegistryObserverInterface::signalTrackedAppIsUpdating, this,
            &SteamHandler::slotTrackedAppIsUpdating);
    connect(&m_steam_close_timer, &QTimer::timeout, this, &SteamHandler::slotTerminateSteam);

    m_steam_close_timer.setSingleShot(true);
}

//---------------------------------------------------------------------------------------------------------------------

bool SteamHandler::isRunning() const
{
    return m_process_handler->isRunning();
}

//---------------------------------------------------------------------------------------------------------------------

bool SteamHandler::isRunningNow()
{
    return m_process_handler->isRunningNow();
}

//---------------------------------------------------------------------------------------------------------------------

bool SteamHandler::close(std::optional<uint> grace_period_in_sec)
{
    if (!m_process_handler->isRunningNow())
    {
        m_steam_close_timer.stop();
        return true;
    }

    // Try to shutdown steam gracefully first
    if (!m_steam_exec_path.isEmpty())
    {
        const auto result = QProcess::startDetached(m_steam_exec_path, {"-shutdown"});
        if (!result)
        {
            qCWarning(lc::os) << "Failed to start Steam shutdown sequence! Using others means to close steam...";
            m_process_handler->close(std::nullopt);
        }
    }
    else
    {
        qCWarning(lc::os) << "Steam EXEC path is not available yet, using other means of closing!";
        m_process_handler->close(std::nullopt);
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

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-cognitive-complexity)
bool SteamHandler::launchApp(uint app_id)
{
    if (m_steam_exec_path.isEmpty())
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
        const bool is_steam_running{m_process_handler->isRunningNow()};
        if (is_steam_running)
        {
            QProcess steam_process;
            steam_process.setStandardOutputFile(QProcess::nullDevice());
            steam_process.setStandardErrorFile(QProcess::nullDevice());
            steam_process.setProgram(m_steam_exec_path);
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
        steam_process.setProgram(m_steam_exec_path);
        steam_process.setArguments((is_steam_running ? QStringList{} : QStringList{"-bigpicture"})
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

//---------------------------------------------------------------------------------------------------------------------

uint SteamHandler::getRunningApp() const
{
    return m_process_handler->isRunning()
               ? m_tracked_app && m_tracked_app->m_is_running ? m_tracked_app->m_app_id : m_global_app_id
               : 0;
}

//---------------------------------------------------------------------------------------------------------------------

std::optional<uint> SteamHandler::getTrackedActiveApp() const
{
    return m_process_handler->isRunning() && m_tracked_app
                   && (m_tracked_app->m_is_running || m_tracked_app->m_is_updating)
               ? std::make_optional(m_tracked_app->m_app_id)
               : std::nullopt;
}

//---------------------------------------------------------------------------------------------------------------------

std::optional<uint> SteamHandler::getTrackedUpdatingApp() const
{
    return m_process_handler->isRunning() && m_tracked_app && m_tracked_app->m_is_updating
               ? std::make_optional(m_tracked_app->m_app_id)
               : std::nullopt;
}

//---------------------------------------------------------------------------------------------------------------------

void SteamHandler::clearTrackedApp()
{
    m_registry_observer->stopTrackingApp();
    if (m_tracked_app)
    {
        m_tracked_app = std::nullopt;
    }
}

//---------------------------------------------------------------------------------------------------------------------

void SteamHandler::slotSteamProcessDied()
{
    qCDebug(lc::os) << "Steam is no longer running!";

    m_registry_observer->stopAppObservation();
    clearTrackedApp();

    m_steam_close_timer.stop();
    m_global_app_id = 0;

#if defined(Q_OS_LINUX)
    // On linux there is a race condition where the crashed Steam process may leave the reaper process (game) running...
    // Let's kill it for fun!
    const uint forced_termination_ms{10000};
    m_process_handler->closeDetached(QRegularExpression(".*?Steam.+?reaper", QRegularExpression::CaseInsensitiveOption),
                                     forced_termination_ms);
#endif

    emit signalProcessStateChanged();
}

//---------------------------------------------------------------------------------------------------------------------

void SteamHandler::slotSteamExecPath(const QString& path)
{
    m_steam_exec_path = path;
    qCDebug(lc::os) << "Steam exec path:" << m_steam_exec_path;
}

//---------------------------------------------------------------------------------------------------------------------

void SteamHandler::slotSteamPID(uint pid)
{
    const bool currently_running{m_process_handler->isRunning()};
    if (pid == 0)
    {
        if (currently_running)
        {
            qCDebug(lc::os) << "Steam is no longer running according to registry. Waiting for actual shutdown.";
        }
        return;
    }

    if (!m_process_handler->startMonitoring(pid, STEAM_EXEC_PATTERN))
    {
        qCDebug(lc::os) << "Failed to start monitoring Steam process" << pid << "(probably outdated)...";
        if (currently_running)
        {
            Q_ASSERT(m_process_handler->isRunning() == false);
            emit signalProcessStateChanged();
        }
        return;
    }

    if (!currently_running)
    {
        qCDebug(lc::os) << "Steam is running!";
        Q_ASSERT(m_process_handler->isRunning() == true);
        m_registry_observer->startAppObservation();
        emit signalProcessStateChanged();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void SteamHandler::slotGlobalAppId(uint app_id)
{
    if (app_id != m_global_app_id)
    {
        m_global_app_id = app_id;
        qCDebug(lc::os) << "Running appID change detected (via global key):" << m_global_app_id;
    }
}

//---------------------------------------------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------------------------------------------

void SteamHandler::slotTerminateSteam()
{
    qCWarning(lc::os) << "Forcefully killing Steam...";

    const uint time_to_kill{10000};
    m_process_handler->close(time_to_kill);
}
}  // namespace os
