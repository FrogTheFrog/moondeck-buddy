// header file include
#include "steamregistryobserver.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QString REG_STEAM_PATH{R"(HKEY_CURRENT_USER\Software\Valve\Steam)"};
const QString REG_STEAM_APPS_PATH{R"(HKEY_CURRENT_USER\Software\Valve\Steam\Apps)"};
const QString REG_STEAM_PROCESS_PATH{R"(HKEY_CURRENT_USER\Software\Valve\Steam\ActiveProcess)"};
const QString REG_APP_ID{"RunningAppId"};
const QString REG_EXEC{"SteamExe"};
const QString REG_APP_RUNNING{"Running"};
const QString REG_APP_UPDATING{"Updating"};
const QString REG_PID{"pid"};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
SteamRegistryObserver::SteamRegistryObserver()
{
    connect(&m_global_reg_key, &RegKey::signalValuesChanged, this, &SteamRegistryObserver::slotRegistryChanged);
    connect(&m_process_reg_key, &RegKey::signalValuesChanged, this, &SteamRegistryObserver::slotRegistryChanged);
    connect(&m_app_reg_key, &RegKey::signalValuesChanged, this, &SteamRegistryObserver::slotRegistryChanged);

    // Delay opening the keys until the next event loop
    QTimer::singleShot(0, this,
                       [this]()
                       {
                           m_global_reg_key.open(REG_STEAM_PATH, {REG_APP_ID, REG_EXEC});
                           m_process_reg_key.open(REG_STEAM_PROCESS_PATH, {REG_PID});
                       });
}

//---------------------------------------------------------------------------------------------------------------------

void SteamRegistryObserver::startTrackingApp(uint app_id)
{
    m_tracked_app_data = TrackedAppData{app_id, false, false};
    m_app_reg_key.open(REG_STEAM_APPS_PATH + R"(\)" + QString::number(app_id),
                       QStringList{REG_APP_RUNNING, REG_APP_UPDATING});
}

//---------------------------------------------------------------------------------------------------------------------

void SteamRegistryObserver::stopTrackingApp()
{
    m_app_reg_key.close();
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-cognitive-complexity)
void SteamRegistryObserver::slotRegistryChanged(const QMap<QString, QVariant>& changed_values)
{
    if (changed_values.contains(REG_PID))
    {
        const auto old_value{m_pid};
        m_pid = changed_values[REG_PID].isValid() ? changed_values[REG_PID].toUInt() : 0;
        if (m_pid != old_value)
        {
            emit signalSteamPID(m_pid);
        }
    }
    if (changed_values.contains(REG_APP_ID))
    {
        const auto old_value{m_global_app_id};
        m_global_app_id = changed_values[REG_APP_ID].isValid() ? changed_values[REG_APP_ID].toUInt() : 0;
        if (m_global_app_id != old_value)
        {
            emit signalGlobalAppId(m_global_app_id);
        }
    }
    if (changed_values.contains(REG_EXEC))
    {
        const auto old_value{m_steam_exec};
        m_steam_exec = changed_values[REG_EXEC].isValid() ? changed_values[REG_EXEC].toString() : QString();
        if (m_steam_exec != old_value)
        {
            emit signalSteamExecPath(m_steam_exec);
        }
    }
    if (m_tracked_app_data)
    {
        if (changed_values.contains(REG_APP_UPDATING))
        {
            const auto old_value{m_tracked_app_data->m_is_updating};
            m_tracked_app_data->m_is_updating =
                changed_values[REG_APP_UPDATING].isValid() ? changed_values[REG_APP_UPDATING].toBool() : false;
            if (m_tracked_app_data->m_is_updating != old_value)
            {
                emit signalTrackedAppIsUpdating(m_tracked_app_data->m_is_updating);
            }
        }
        if (changed_values.contains(REG_APP_RUNNING))
        {
            const auto old_value{m_tracked_app_data->m_is_running};
            m_tracked_app_data->m_is_running =
                changed_values[REG_APP_RUNNING].isValid() ? changed_values[REG_APP_RUNNING].toBool() : false;
            if (m_tracked_app_data->m_is_running != old_value)
            {
                emit signalTrackedAppIsRunning(m_tracked_app_data->m_is_running);
            }
        }
    }
}
}  // namespace os
