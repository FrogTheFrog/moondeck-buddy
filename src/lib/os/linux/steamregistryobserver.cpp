// header file include
#include "os/linux/steamregistryobserver.h"

// system/Qt includes
#include <QDir>
#include <QFile>
#include <chrono>

// local includes
#include "shared/loggingcategories.h"

namespace
{
const QStringList PID_PATH{{"Registry", "HKLM", "Software", "Valve", "Steam", "SteamPID"}};

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

namespace os
{
SteamRegistryObserver::SteamRegistryObserver(QString registry_file_override, QString steam_binary_override)
    : m_watcher{registry_file_override.isEmpty() ? QDir::homePath() + "/.steam/registry.vdf"
                                                 : std::move(registry_file_override)}
    , m_steam_exec{steam_binary_override.isEmpty() ? "/usr/bin/steam" : std::move(steam_binary_override)}
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

    if (!QFile::exists(m_steam_exec))
    {
        qFatal("Steam binary does not exist at specified path: %s", qUtf8Printable(m_steam_exec));
    }
    else
    {
        qCInfo(lc::os) << "Steam binary path set to" << m_steam_exec;
    }

    const int initial_delay_ms{2000};
    m_observation_delay.setInterval(initial_delay_ms);
    m_observation_delay.setSingleShot(true);
}

void SteamRegistryObserver::startAppObservation()
{
    m_observation_delay.start();
    m_process_list_observer.observePid(m_pid);
}

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

void SteamRegistryObserver::startTrackingApp(uint app_id)
{
    m_tracked_app_data = TrackedAppData{app_id, false, false};
    slotRegistryChanged();
}

void SteamRegistryObserver::stopTrackingApp()
{
    m_tracked_app_data = std::nullopt;
}

// NOLINTNEXTLINE(*-cognitive-complexity)
void SteamRegistryObserver::slotRegistryChanged()
{
    const auto& data{m_watcher.getData()};
    const auto  get_uint = [](const qint64* ptr) { return ptr == nullptr ? 0 : static_cast<uint>(*ptr); };

    auto pid{get_uint(getEntry<qint64>(PID_PATH, data))};
    if (pid != m_pid)
    {
        if (pid != 0)
        {
            const uint actual_steam_pid{m_process_list_observer.findSteamProcess(m_pid)};
            if (actual_steam_pid == 0)
            {
                using namespace std::chrono_literals;
                const bool do_recheck{m_recheck_counter++ < 10};

                qCWarning(lc::os).nospace() << "Steam PID from registry.vdf indicates that the Steam process is "
                                               "running, but it's not"
                                            << (do_recheck ? "... Rechecking in 5 seconds." : "...");

                if (do_recheck)
                {
                    QTimer::singleShot(5s, this, &SteamRegistryObserver::slotRegistryChanged);
                }
                else
                {
                    // Something else has triggered the slots, let's reset the counter.
                    m_recheck_counter = 0;
                }
            }
            else if (actual_steam_pid != pid)
            {
                if (m_pid != actual_steam_pid)
                {
                    qCWarning(lc::os).nospace()
                        << "Steam PID from registry.vdf does not match what we have found (normal for flatpak or "
                           "outdated data)! Using PID "
                        << actual_steam_pid << " (instead of " << pid << ") to track Steam process.";
                }
            }

            if (actual_steam_pid != 0)
            {
                m_recheck_counter = 0;
            }

            pid = actual_steam_pid;
        }
        else
        {
            m_recheck_counter = 0;
        }

        if (pid != m_pid)
        {
            m_pid = pid;

            m_process_list_observer.observePid(m_pid);
            emit signalSteamPID(m_pid);
        }
    }

    if (!m_steam_exec.isEmpty())
    {
        emit signalSteamExecPath(m_steam_exec);
        m_steam_exec = {};  // Reset to no longer trigger this conditional
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
