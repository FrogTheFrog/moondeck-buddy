// header file include
#include "steamhandler.h"

// system/Qt includes
#include <QProcess>

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
SteamHandler::SteamHandler(std::shared_ptr<ProcessEnumerator>& enumerator)
    : m_steam_process{QRegularExpression{R"([\\\/]steam\.exe$)", QRegularExpression::CaseInsensitiveOption}, enumerator}
{
    connect(&m_steam_process, &ProcessTracker::signalProcessStateChanged, this,
            &SteamHandler::slotSteamProcessStateChanged);

    connect(&m_global_reg_key, &RegKey::signalValuesChanged, this, &SteamHandler::slotHandleGlobalRegKeyChanges);
    m_global_reg_key.open(REG_STEAM_PATH, {REG_APP_ID, REG_EXEC});

    connect(&m_steam_close_timer, &QTimer::timeout, this, &SteamHandler::slotForceCloseSteam);
    m_steam_close_timer.setSingleShot(true);
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

void SteamHandler::close(std::optional<uint> grace_period_in_sec)
{
    if (!m_steam_process.isRunningNow())
    {
        m_steam_close_timer.stop();
        return;
    }

    // Try to shutdown steam gracefully first
    if (!m_steam_exec_path.isEmpty())
    {
        const auto result = QProcess::startDetached(m_steam_exec_path, {"-shutdown"});
        if (!result)
        {
            qWarning("Failed to start Steam shutdown sequence!");
            m_steam_process.close();
            return;
        }
    }
    else
    {
        qWarning("Steam EXEC path is not available yet, using other means of closing!");
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
}

//---------------------------------------------------------------------------------------------------------------------

void SteamHandler::launchApp(uint app_id, const QStringList& steam_args)
{
    if (m_steam_exec_path.isEmpty())
    {
        qWarning("Steam EXEC path is not available yet!");
        return;
    }

    if (m_steam_close_timer.isActive())
    {
        qWarning("Already closing Steam, will not launch new app!");
        return;
    }

    if (app_id == FALLBACK_APP_ID)
    {
        qWarning("Will not launch app with 0 ID!");
        return;
    }

    if (!m_steam_process.isRunningNow() || getRunningApp() != app_id)
    {
        const QStringList args{steam_args + QStringList{"-applaunch", QString::number(app_id)}};
        const auto  result = QProcess::startDetached(m_steam_exec_path, args);
        if (!result)
        {
            qWarning("Failed to start Steam app launch sequence!");
            return;
        }

        m_app_reg_key = std::make_unique<RegKey>();
        connect(m_app_reg_key.get(), &RegKey::signalValuesChanged, this,
                [this, app_id](const QMap<QString, QVariant>& changed_values)
                { slotHandleAppRegKeyChanges(changed_values, app_id); });

        m_app_reg_key->open(REG_STEAM_APPS_PATH + R"(\)" + QString::number(app_id),
                            QStringList{REG_APP_RUNNING, REG_APP_UPDATING});
    }
}

//---------------------------------------------------------------------------------------------------------------------

uint SteamHandler::getRunningApp() const
{
    return m_tracked_app && m_tracked_app->m_is_running ? m_tracked_app->m_app_id
                                                        : m_global_app_id.value_or(FALLBACK_APP_ID);
}

//---------------------------------------------------------------------------------------------------------------------

std::optional<uint> SteamHandler::isLastLaunchedAppUpdating() const
{
    return m_tracked_app && m_tracked_app->m_is_updating ? std::make_optional(m_tracked_app->m_app_id) : std::nullopt;
}

//---------------------------------------------------------------------------------------------------------------------

void SteamHandler::slotSteamProcessStateChanged()
{
    if (m_steam_process.isRunning())
    {
        qDebug("Steam is running!");
    }
    else
    {
        qDebug("Steam is not running!");
        m_global_app_id = std::nullopt;
        m_tracked_app   = std::nullopt;
        m_app_reg_key   = nullptr;
        m_steam_close_timer.stop();
    }

    emit signalSteamStateChanged();
}

//---------------------------------------------------------------------------------------------------------------------

void SteamHandler::slotHandleGlobalRegKeyChanges(const QMap<QString, QVariant>& changed_values)
{
    if (changed_values.contains(REG_EXEC))
    {
        m_steam_exec_path = changed_values[REG_EXEC].isValid() ? changed_values[REG_EXEC].toString() : QString();
        qDebug("Steam exec path: %s", qUtf8Printable(m_steam_exec_path));
    }
    if (changed_values.contains(REG_APP_ID))
    {
        const auto prev_app_id = m_global_app_id;
        m_global_app_id = changed_values[REG_APP_ID].isValid() ? std::make_optional(changed_values[REG_APP_ID].toUInt())
                                                               : std::nullopt;

        if (prev_app_id != m_global_app_id)
        {
            qDebug("Running appID change detected (via global key): %s",
                   qUtf8Printable(QString::number(m_global_app_id.value_or(FALLBACK_APP_ID))));
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
        qDebug("App %s \"running\" value change detected: %s", qUtf8Printable(QString::number(app_id)),
               qUtf8Printable(is_running ? QStringLiteral("true") : QStringLiteral("false")));
    }
    if (changed_values.contains(REG_APP_UPDATING))
    {
        is_updating = (changed_values[REG_APP_UPDATING].isValid() ? changed_values[REG_APP_UPDATING].toBool() : false);
        qDebug("App %s \"updating\" value change detected: %s", qUtf8Printable(QString::number(app_id)),
               qUtf8Printable(is_updating ? QStringLiteral("true") : QStringLiteral("false")));
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
