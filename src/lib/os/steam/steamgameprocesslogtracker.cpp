// header file include
#include "os/steam/steamgameprocesslogtracker.h"

// system/Qt includes
#include <QRegularExpression>

// local includes
#include "shared/loggingcategories.h"

namespace os
{
SteamGameProcessLogTracker::SteamGameProcessLogTracker(const std::filesystem::path& logs_dir,
                                                       QDateTime                    first_entry_time_filter)
    : SteamLogTracker(logs_dir / "gameprocess_log.txt", logs_dir / "gameprocess_log.previous.txt",
                      std::move(first_entry_time_filter))
{
}

bool SteamGameProcessLogTracker::isAnyProcessRunning(const uint app_id) const
{
    return m_app_id_to_process_ids.contains(app_id);
}

void SteamGameProcessLogTracker::onLogChanged(const std::vector<QString>& new_lines)
{
    std::set<uint> added_app_ids;
    std::set<uint> removed_app_ids;

    for (const QString& line : new_lines)
    {
        static const QRegularExpression add_regex{R"(AppID (\d+) adding PID (\d+))"};
        if (const auto match{add_regex.match(line)}; match.hasMatch())
        {
            const auto app_id{match.captured(1).toUInt()};
            if (app_id == 0)
            {
                qCWarning(lc::os) << "Failed to get AppID from" << line;
                continue;
            }

            const auto pid{match.captured(2).toUInt()};
            if (pid == 0)
            {
                qCWarning(lc::os) << "Failed to get PID from" << line;
                continue;
            }

            m_app_id_to_process_ids[app_id].insert(pid);
            added_app_ids.insert(app_id);
            removed_app_ids.erase(app_id);
            continue;
        }

        static const QRegularExpression remove_regex{
            R"((?:Game (\d+) going away.* PID (\d+))|(?:AppID (\d+) no longer.* PID (\d+)))"};
        if (const auto match{remove_regex.match(line)}; match.hasMatch())
        {
            const auto app_id{(match.hasCaptured(1) ? match.captured(1) : match.captured(3)).toUInt()};
            if (app_id == 0)
            {
                qCWarning(lc::os) << "Failed to get AppID from" << line;
                continue;
            }

            const auto pid{(match.hasCaptured(2) ? match.captured(2) : match.captured(4)).toUInt()};
            if (pid == 0)
            {
                qCWarning(lc::os) << "Failed to get PID from" << line;
                continue;
            }

            auto data_it = m_app_id_to_process_ids.find(app_id);
            if (data_it == std::end(m_app_id_to_process_ids))
            {
                qCWarning(lc::os) << "Trying to remove PID" << pid << "from" << app_id << "but AppID is not tracked!";
                continue;
            }

            data_it->second.erase(pid);
            if (data_it->second.empty())
            {
                removed_app_ids.insert(app_id);
                added_app_ids.erase(app_id);
            }
        }
    }

    for (const auto& app_id : added_app_ids)
    {
        qCInfo(lc::os) << "Running processes added for AppID:" << app_id;
    }

    for (const auto& app_id : removed_app_ids)
    {
        qCInfo(lc::os) << "Running processes removed for AppID:" << app_id;
    }
}
}  // namespace os
