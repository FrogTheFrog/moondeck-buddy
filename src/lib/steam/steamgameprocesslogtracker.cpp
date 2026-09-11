// header file include
#include "steam/steamgameprocesslogtracker.h"

// system/Qt includes
#include <QDebug>
#include <QRegularExpression>

// local includes
#include "common/loggingcategories.h"

namespace steam
{
SteamGameProcessLogTracker::SteamGameProcessLogTracker(const std::filesystem::path& logs_dir,
                                                       QDateTime                    first_entry_time_filter)
    : SteamLogTracker(logs_dir / "gameprocess_log.txt", logs_dir / "gameprocess_log.previous.txt",
                      std::move(first_entry_time_filter))
{
}

SteamGameProcessLogTracker::~SteamGameProcessLogTracker()
{
    if (!m_app_id_to_process_ids.empty())
    {
        m_app_id_to_process_ids.clear();
        emit signalStateChanged();
    }
}

bool SteamGameProcessLogTracker::isAnyProcessRunning(const AppId& app_id) const
{
    return m_app_id_to_process_ids.contains(app_id);
}

std::map<uint, QDateTime> SteamGameProcessLogTracker::getTrackedProcesses(const AppId& app_id) const
{
    std::map<uint, QDateTime> result;
    if (const auto it{m_app_id_to_process_ids.find(app_id)}; it != m_app_id_to_process_ids.end())
    {
        for (const uint pid : it->second)
        {
            if (const auto time{m_process_added_at.find(pid)}; time != m_process_added_at.end())
            {
                result.emplace(pid, time->second);
            }
        }
    }
    return result;
}

void SteamGameProcessLogTracker::onLogChanged(const std::vector<QString>& new_lines)
{
    const auto try_emplace_pid_list{[](std::map<AppId, QSet<uint>>& container, const AppId& app_id,
                                       const QSet<uint>& default_list = {}) -> QSet<uint>&
                                    { return container.try_emplace(app_id, default_list).first->second; }};

    std::map<AppId, QSet<uint>> initial_entries;
    for (const QString& line : new_lines)
    {
        static const QRegularExpression add_regex{R"(AppID (\d+) adding PID (\d+))"};
        if (const auto match{add_regex.match(line)}; match.hasMatch())
        {
            const auto app_id{AppId::fromString(match.captured(1))};
            if (!app_id)
            {
                qCWarning(lc::steam) << "Failed to get AppID from" << line;
                continue;
            }

            const auto pid{match.captured(2).toUInt()};
            if (pid == 0)
            {
                qCWarning(lc::steam) << "Failed to get PID from" << line;
                continue;
            }

            auto& current_pids{try_emplace_pid_list(m_app_id_to_process_ids, *app_id)};

            try_emplace_pid_list(initial_entries, *app_id, current_pids);
            current_pids.insert(pid);
            // Steam timestamps have one-second precision. Keep the registration time
            // so a stale log entry cannot identify a newer, recycled process ID.
            m_process_added_at[pid] = QDateTime::fromString(line.mid(1, 19), "yyyy-MM-dd HH:mm:ss");
            continue;
        }

        static const QRegularExpression remove_regex{
            R"((?:Game \d+ going away.* PID (\d+))|(?:AppID \d+ no longer.* PID (\d+)))"};
        if (const auto match{remove_regex.match(line)}; match.hasMatch())
        {
            const auto pid{(match.hasCaptured(1) ? match.captured(1) : match.captured(2)).toUInt()};
            if (pid == 0)
            {
                qCWarning(lc::steam) << "Failed to get PID from" << line;
                continue;
            }

            qCDebug(lc::steam) << "Removing PID" << pid << "from all tracked AppIDs";
            m_process_added_at.erase(pid);
            for (auto& [app_id, current_pids] : m_app_id_to_process_ids)
            {
                try_emplace_pid_list(initial_entries, app_id, current_pids);
                current_pids.remove(pid);
            }
        }
    }

    bool current_state_changed{false};
    for (const auto& [app_id, initial_pids] : initial_entries)
    {
        auto data_it{m_app_id_to_process_ids.find(app_id)};
        if (data_it == m_app_id_to_process_ids.end())
        {
            qFatal(lc::steam) << "AppID not found: " << app_id.getId();
        }

        if (data_it->second != initial_pids)
        {
            qCDebug(lc::steam) << "Running processes changed for AppID:" << app_id.getId() << "->" << data_it->second;
            current_state_changed = true;

            if (initial_pids.empty())
            {
                qCInfo(lc::steam) << "Running processes added for AppID:" << app_id.getId();
            }
            else if (data_it->second.empty())
            {
                qCInfo(lc::steam) << "Running processes removed for AppID:" << app_id.getId();
            }
        }

        if (data_it->second.empty())
        {
            m_app_id_to_process_ids.erase(data_it);
        }
    }

    if (current_state_changed)
    {
        emit signalStateChanged();
    }
}
}  // namespace steam
