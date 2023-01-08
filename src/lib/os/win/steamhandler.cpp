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
const QString REG_STEAM_PATH{R"(HKEY_CURRENT_USER\Software\Valve\Steam)"};
const QString REG_STEAM_APPS_PATH{R"(HKEY_CURRENT_USER\Software\Valve\Steam\Apps)"};
const QString REG_APP_ID{"RunningAppId"};
const QString REG_EXEC{"SteamExe"};
const QString REG_APP_RUNNING{"Running"};
const QString REG_APP_UPDATING{"Updating"};

const uint FALLBACK_APP_ID{0};
const int  MS_TO_SEC{1000};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
SteamHandler::SteamHandler()
    : m_enumerator{std::make_shared<ProcessEnumerator>()}
    , m_steam_process{QRegularExpression{R"([\\\/]steam\.exe$)", QRegularExpression::CaseInsensitiveOption},
                      m_enumerator}
{
    connect(&m_steam_process, &ProcessTracker::signalProcessStateChanged, this,
            &SteamHandler::slotSteamProcessStateChanged);

    connect(&m_global_reg_key, &RegKey::signalValuesChanged, this, &SteamHandler::slotHandleGlobalRegKeyChanges);
    m_global_reg_key.open(REG_STEAM_PATH, {REG_APP_ID, REG_EXEC});

    connect(&m_steam_close_timer, &QTimer::timeout, this, &SteamHandler::slotForceCloseSteam);
    m_steam_close_timer.setSingleShot(true);

    const auto default_enumeration_interval{1000};
    m_enumerator->start(default_enumeration_interval);
}

//---------------------------------------------------------------------------------------------------------------------

bool SteamHandler::isRunning() const
{
    return m_steam_process.isRunning();
}

//---------------------------------------------------------------------------------------------------------------------

bool SteamHandler::isRunningNow()
{
    return m_steam_process.isRunningNow();
}

//---------------------------------------------------------------------------------------------------------------------

bool SteamHandler::close(std::optional<uint> grace_period_in_sec)
{
    if (!m_steam_process.isRunningNow())
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
            m_steam_process.close();
        }
    }
    else
    {
        qCWarning(lc::os) << "Steam EXEC path is not available yet, using other means of closing!";
        m_steam_process.close();
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

    if (app_id == FALLBACK_APP_ID)
    {
        qCWarning(lc::os) << "Will not launch app with 0 ID!";
        return false;
    }

    if (getRunningApp() != app_id)
    {
        const bool        is_steam_running{m_steam_process.isRunningNow()};
        const QStringList steam_args{(is_steam_running ? QStringList{} : QStringList{"-bigpicture"})
                                     + QStringList{"-applaunch", QString::number(app_id)}};

        if (is_steam_running)
        {
            const auto result = QProcess::startDetached(m_steam_exec_path, {"steam://open/bigpicture"});
            if (!result)
            {
                qCWarning(lc::os) << "Failed to open Steam in big picture mode!";
                return false;
            }

            // Need to wait a little until Steam actually goes into bigpicture mode...
            const uint time_it_takes_steam_to_open_big_picture{1000};
            QThread::msleep(time_it_takes_steam_to_open_big_picture);
        }

        const auto result = QProcess::startDetached(m_steam_exec_path, steam_args);
        if (!result)
        {
            qCWarning(lc::os) << "Failed to start Steam app launch sequence!";
            return false;
        }

        m_app_reg_key = std::make_unique<RegKey>();
        connect(m_app_reg_key.get(), &RegKey::signalValuesChanged, this,
                [this, app_id](const QMap<QString, QVariant>& changed_values)
                { slotHandleAppRegKeyChanges(changed_values, app_id); });

        m_app_reg_key->open(REG_STEAM_APPS_PATH + R"(\)" + QString::number(app_id),
                            QStringList{REG_APP_RUNNING, REG_APP_UPDATING});
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------

uint SteamHandler::getRunningApp() const
{
    return m_tracked_app && m_tracked_app->m_is_running ? m_tracked_app->m_app_id
                                                        : m_global_app_id.value_or(FALLBACK_APP_ID);
}

//---------------------------------------------------------------------------------------------------------------------

std::optional<uint> SteamHandler::getTrackedUpdatingApp() const
{
    return m_tracked_app && m_tracked_app->m_is_updating ? std::make_optional(m_tracked_app->m_app_id) : std::nullopt;
}

//---------------------------------------------------------------------------------------------------------------------

void SteamHandler::slotSteamProcessStateChanged()
{
    if (m_steam_process.isRunning())
    {
        qCDebug(lc::os) << "Steam is running!";
    }
    else
    {
        qCDebug(lc::os) << "Steam is not running!";
        m_global_app_id = std::nullopt;
        m_tracked_app   = std::nullopt;
        m_app_reg_key   = nullptr;
        m_steam_close_timer.stop();
    }

    emit signalProcessStateChanged();
}

//---------------------------------------------------------------------------------------------------------------------

void SteamHandler::slotHandleGlobalRegKeyChanges(const QMap<QString, QVariant>& changed_values)
{
    if (changed_values.contains(REG_EXEC))
    {
        m_steam_exec_path = changed_values[REG_EXEC].isValid() ? changed_values[REG_EXEC].toString() : QString();
        qCDebug(lc::os) << "Steam exec path:" << m_steam_exec_path;
    }
    if (changed_values.contains(REG_APP_ID))
    {
        const auto prev_app_id = m_global_app_id;
        m_global_app_id = changed_values[REG_APP_ID].isValid() ? std::make_optional(changed_values[REG_APP_ID].toUInt())
                                                               : std::nullopt;

        if (prev_app_id != m_global_app_id)
        {
            qCDebug(lc::os) << "Running appID change detected (via global key):"
                            << m_global_app_id.value_or(FALLBACK_APP_ID);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

void SteamHandler::slotHandleAppRegKeyChanges(const QMap<QString, QVariant>& changed_values, uint app_id)
{
    bool is_running{false};
    bool is_updating{false};

    if (changed_values.contains(REG_APP_RUNNING))
    {
        is_running = (changed_values[REG_APP_RUNNING].isValid() ? changed_values[REG_APP_RUNNING].toBool() : false);
        qCDebug(lc::os).nospace() << "App " << app_id << " \"running\" value change detected: " << is_running;
    }
    if (changed_values.contains(REG_APP_UPDATING))
    {
        is_updating = (changed_values[REG_APP_UPDATING].isValid() ? changed_values[REG_APP_UPDATING].toBool() : false);
        qCDebug(lc::os).nospace() << "App " << app_id << " \"updating\" value change detected: " << is_running;
    }

    if (m_tracked_app && m_tracked_app->m_is_running && !is_running)
    {
        // The app was closed, we are no longer watching it
        m_tracked_app = std::nullopt;
        m_app_reg_key = nullptr;
        return;
    }

    m_tracked_app = TrackedAppData{app_id, is_running, is_updating};
}

//---------------------------------------------------------------------------------------------------------------------

void SteamHandler::slotForceCloseSteam()
{
    m_steam_process.terminate();
}
}  // namespace os
