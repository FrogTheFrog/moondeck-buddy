// header file include
#include "steam/steamgameprocesslogtracker.h"

// system/Qt includes
#include <QDebug>
#include <QRegularExpression>
#include <ranges>

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

const SteamGameProcessLogTracker::AppIdToPidDataMap& SteamGameProcessLogTracker::getAppIdData() const
{
    return m_app_id_to_process_ids;
}

void SteamGameProcessLogTracker::onLogChanged(const std::vector<LogLine>& new_lines)
{
    static const auto get_or_create_pid_data{[](AppIdToPidDataMap& container, const AppId& app_id) -> PidDataMap&
                                             { return container.try_emplace(app_id, PidDataMap{}).first->second; }};
    static const auto get_pids{
        [](const PidDataMap& pid_data)
        { return pid_data.asKeyValueRange() | std::views::keys | std::ranges::to<QSet<uint>>(); }};
    static const auto try_emplace_pid_list{[](auto& container, const AppId& app_id, const PidDataMap& pid_data)
                                           {
                                               // Small optimization to avoid getting keys from data
                                               if (!container.contains(app_id))
                                               {
                                                   container[app_id] = get_pids(pid_data);
                                               }
                                           }};

    std::map<AppId, QSet<uint>> initial_entries;
    for (const auto& line : new_lines)
    {
        static const QRegularExpression add_regex{R"(AppID (\d+) adding PID (\d+))"};
        if (const auto match{add_regex.match(line.m_text)}; match.hasMatch())
        {
            const auto app_id{AppId::fromString(match.captured(1))};
            if (!app_id)
            {
                qCWarning(lc::steam) << "Failed to get AppID from" << line.m_text;
                continue;
            }

            const auto pid{match.captured(2).toUInt()};
            if (pid == 0)
            {
                qCWarning(lc::steam) << "Failed to get PID from" << line.m_text;
                continue;
            }

            auto& current_pids{get_or_create_pid_data(m_app_id_to_process_ids, *app_id)};
            try_emplace_pid_list(initial_entries, *app_id, current_pids);

            const auto timestamp{line.parseTimestamp(getDefaultTimeFormat())};
            if (!timestamp.isValid())
            {
                qCWarning(lc::steam) << "PID contains invalid timestamp (still storing the PID)" << line.m_text;
            }

            current_pids[pid] = timestamp;
            continue;
        }

        static const QRegularExpression remove_regex{
            R"((?:Game \d+ going away.* PID (\d+))|(?:AppID \d+ no longer.* PID (\d+)))"};
        if (const auto match{remove_regex.match(line.m_text)}; match.hasMatch())
        {
            const auto pid{(match.hasCaptured(1) ? match.captured(1) : match.captured(2)).toUInt()};
            if (pid == 0)
            {
                qCWarning(lc::steam) << "Failed to get PID from" << line.m_text;
                continue;
            }

            qCDebug(lc::steam) << "Removing PID" << pid << "from all tracked AppIDs";
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

        // We are ignoring the timestamp here for now. They should not change and even if they do - we do not care about
        // the timestamp diff for now...
        if (const auto current_pids{get_pids(data_it->second)}; current_pids != initial_pids)
        {
            qCDebug(lc::steam) << "Running processes changed for AppID:" << app_id.getId() << "->" << current_pids;
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
