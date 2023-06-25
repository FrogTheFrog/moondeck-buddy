// header file include
#include "steamregistryobserver.h"

// system/Qt includes
#include <QDir>
#include <QFile>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QStringList PID_PATH{{"Registry", "HKLM", "Software", "Valve", "Steam", "SteamPID"}};

//---------------------------------------------------------------------------------------------------------------------

template<class T>
const T* getEntry(const QStringList& path, const os::RegistryFileWatcher::NodeList& node_list)
{
    if (path.isEmpty() || node_list.empty())
    {
        return nullptr;
    }

    const os::RegistryFileWatcher::NodeList* current_node_list{&node_list};
    auto                                     current_segment{path.cbegin()};

    for (std::size_t i = 0; i < current_node_list->size();)
    {
        const auto& node{(*current_node_list)[i]};
        Q_ASSERT(node != nullptr);

        if (node->m_key != *current_segment)
        {
            ++i;
            continue;
        }

        current_segment++;
        if (current_segment != path.cend())
        {
            i                 = 0;
            current_node_list = std::get_if<os::RegistryFileWatcher::NodeList>(&node->m_value);

            if (current_node_list == nullptr)
            {
                break;
            }
        }
        else
        {
            return std::get_if<T>(&node->m_value);
        }
    }

    return nullptr;
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
SteamRegistryObserver::SteamRegistryObserver()
    : m_watcher{QDir::homePath() + "/.steam/registry.vdf"}
{
    connect(&m_watcher, &RegistryFileWatcher::signalRegistryChanged, this, &SteamRegistryObserver::slotRegistryChanged);
    connect(&m_process_list_observer, &SteamProcessListObserver::signalListChanged, this,
            &SteamRegistryObserver::slotRegistryChanged);
    connect(&m_observation_delay, &QTimer::timeout, this,
            [this]()
            {
                m_is_observing_apps = true;
                slotRegistryChanged();
            });

    const int initial_delay_ms{2000};
    m_observation_delay.setInterval(initial_delay_ms);
    m_observation_delay.setSingleShot(true);
}

//---------------------------------------------------------------------------------------------------------------------

void SteamRegistryObserver::startAppObservation()
{
    m_observation_delay.start();
    m_process_list_observer.observePid(m_pid);
}

//---------------------------------------------------------------------------------------------------------------------

void SteamRegistryObserver::stopAppObservation()
{
    m_observation_delay.stop();
    m_process_list_observer.stopObserving();

    m_is_observing_apps = false;
    m_global_app_id     = 0;
    if (m_tracked_app_data)
    {
        m_tracked_app_data->m_is_running  = false;
        m_tracked_app_data->m_is_updating = false;
    }
}

//---------------------------------------------------------------------------------------------------------------------

void SteamRegistryObserver::startTrackingApp(uint app_id)
{
    m_tracked_app_data = TrackedAppData{app_id, false, false};
    slotRegistryChanged();
}

//---------------------------------------------------------------------------------------------------------------------

void SteamRegistryObserver::stopTrackingApp()
{
    m_tracked_app_data = std::nullopt;
}

//---------------------------------------------------------------------------------------------------------------------

void SteamRegistryObserver::slotRegistryChanged()
{
    const auto& data{m_watcher.getData()};
    const auto  get_uint = [](const qint64* ptr) { return ptr == nullptr ? 0 : static_cast<uint>(*ptr); };

    const auto pid{get_uint(getEntry<qint64>(PID_PATH, data))};
    if (pid != m_pid)
    {
        m_pid = pid;

        m_process_list_observer.observePid(m_pid);
        emit signalSteamPID(m_pid);
    }

    if (m_steam_exec.isEmpty())
    {
        const QString hardcoded_path{"/usr/bin/steam"};
        if (QFile::exists(hardcoded_path))
        {
            m_steam_exec = hardcoded_path;
            emit signalSteamExecPath(m_steam_exec);
        }
    }

    if (!m_is_observing_apps)
    {
        return;
    }

    // Note: everything after here is a workaround, because Steam is no longer saving stuff to the registry
    //       https://github.com/ValveSoftware/steam-for-linux/issues/9672

    const auto running_apps{m_process_list_observer.getAppIds()};
    if (m_tracked_app_data)
    {
        const bool tracked_app_is_running{running_apps.contains(m_tracked_app_data->m_app_id)};
        const uint global_app_id{tracked_app_is_running ? m_tracked_app_data->m_app_id
                                 : running_apps.empty() ? 0
                                                        : *running_apps.begin()};

        if (global_app_id != m_global_app_id)
        {
            m_global_app_id = global_app_id;
            emit signalGlobalAppId(m_global_app_id);
        }

        if (tracked_app_is_running != m_tracked_app_data->m_is_running)
        {
            m_tracked_app_data->m_is_running = tracked_app_is_running;
            emit signalTrackedAppIsRunning(m_tracked_app_data->m_is_running);
        }
    }
    else
    {
        const uint global_app_id{running_apps.empty() ? 0 : *running_apps.begin()};
        if (global_app_id != m_global_app_id)
        {
            m_global_app_id = global_app_id;
            emit signalGlobalAppId(m_global_app_id);
        }
    }
}
}  // namespace os
