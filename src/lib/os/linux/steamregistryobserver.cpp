// header file include
#include "steamregistryobserver.h"

// system/Qt includes
#include <QDir>
#include <QFile>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QStringList PID_PATH{{"Registry", "HKLM", "Software", "Valve", "Steam", "SteamPID"}};
const QStringList GLOBAL_APP_ID_PATH{{"Registry", "HKCU", "Software", "Valve", "Steam", "RunningAppID"}};
const QStringList APPS_PATH{{"Registry", "HKCU", "Software", "Valve", "Steam", "Apps"}};
const QStringList APP_RUNNING_PATH{{"Running"}};
const QStringList APP_UPDATING_PATH{{"Updating"}};

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
        assert(node != nullptr);

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
    const auto  get_bool = [](const qint64* ptr) { return ptr == nullptr ? false : *ptr != 0; };

    const auto pid{get_uint(getEntry<qint64>(PID_PATH, data))};
    if (pid != m_pid)
    {
        m_pid = pid;
        emit signalSteamPID(m_pid);
    }

    const auto global_app_id{get_uint(getEntry<qint64>(GLOBAL_APP_ID_PATH, data))};
    if (global_app_id != m_global_app_id)
    {
        m_global_app_id = global_app_id;
        emit signalGlobalAppId(m_global_app_id);
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

    if (m_tracked_app_data)
    {
        const auto* app_data{getEntry<os::RegistryFileWatcher::NodeList>(
            APPS_PATH + QStringList{QString::number(m_tracked_app_data->m_app_id)}, data)};

        bool is_updating{app_data == nullptr ? false : get_bool(getEntry<qint64>(APP_UPDATING_PATH, *app_data))};
        if (is_updating != m_tracked_app_data->m_is_updating)
        {
            m_tracked_app_data->m_is_updating = is_updating;
            emit signalTrackedAppIsRunning(m_tracked_app_data->m_is_updating);
        }

        bool is_running{app_data == nullptr ? false : get_bool(getEntry<qint64>(APP_RUNNING_PATH, *app_data))};
        if (is_running != m_tracked_app_data->m_is_running)
        {
            m_tracked_app_data->m_is_running = is_running;
            emit signalTrackedAppIsRunning(m_tracked_app_data->m_is_running);
        }
    }
}
}  // namespace os
