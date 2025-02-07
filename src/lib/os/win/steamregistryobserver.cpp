// header file include
#include "os/win/steamregistryobserver.h"

namespace
{
const QString REG_STEAM_PATH{R"(HKEY_CURRENT_USER\Software\Valve\Steam)"};
const QString REG_STEAM_APPS_PATH{R"(HKEY_CURRENT_USER\Software\Valve\Steam\Apps)"};
const QString REG_APP_ID{"RunningAppId"};
const QString REG_APP_RUNNING{"Running"};
const QString REG_APP_UPDATING{"Updating"};
}  // namespace

namespace os
{
SteamRegistryObserver::SteamRegistryObserver(QString registry_file_override)
{
    Q_UNUSED(registry_file_override);

    connect(&m_global_reg_key, &RegKey::signalValuesChanged, this, &SteamRegistryObserver::slotRegistryChanged);
    connect(&m_steam_exec_reg_key, &RegKey::signalValuesChanged, this, &SteamRegistryObserver::slotRegistryChanged);
    connect(&m_process_reg_key, &RegKey::signalValuesChanged, this, &SteamRegistryObserver::slotRegistryChanged);
    connect(&m_app_reg_key, &RegKey::signalValuesChanged, this, &SteamRegistryObserver::slotRegistryChanged);
    connect(&m_observation_delay, &QTimer::timeout, this,
            [this]()
            {
                m_is_observing_apps = true;
                m_global_reg_key.open(REG_STEAM_PATH, {REG_APP_ID});
                if (m_tracked_app_data)
                {
                    m_app_reg_key.open(REG_STEAM_APPS_PATH + R"(\)" + QString::number(m_tracked_app_data->m_app_id),
                                       QStringList{REG_APP_RUNNING, REG_APP_UPDATING});
                }
            });

    const int initial_delay_ms{2000};
    m_observation_delay.setInterval(initial_delay_ms);
    m_observation_delay.setSingleShot(true);
}

void SteamRegistryObserver::startAppObservation()
{
    m_observation_delay.start();
}

void SteamRegistryObserver::stopAppObservation()
{
    m_observation_delay.stop();
    m_is_observing_apps = false;
    m_global_app_id     = 0;
    if (m_tracked_app_data)
    {
        m_tracked_app_data->m_is_running  = false;
        m_tracked_app_data->m_is_updating = false;
    }
}

void SteamRegistryObserver::startTrackingApp(uint app_id)
{
    m_tracked_app_data = TrackedAppData{app_id, false, false};
    if (m_is_observing_apps)
    {
        m_app_reg_key.open(REG_STEAM_APPS_PATH + R"(\)" + QString::number(app_id),
                           QStringList{REG_APP_RUNNING, REG_APP_UPDATING});
    }
}

void SteamRegistryObserver::stopTrackingApp()
{
    m_app_reg_key.close();
}

// NOLINTNEXTLINE(*-cognitive-complexity)
void SteamRegistryObserver::slotRegistryChanged(const QMap<QString, QVariant>& changed_values)
{
    if (changed_values.contains(REG_APP_ID))
    {
        const auto old_value{m_global_app_id};
        m_global_app_id = changed_values[REG_APP_ID].isValid() ? changed_values[REG_APP_ID].toUInt() : 0;
        if (m_global_app_id != old_value)
        {
            emit signalGlobalAppId(m_global_app_id);
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
