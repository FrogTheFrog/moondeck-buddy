// header file include
#include "steam/steamgameprocesslogtracker.h"

// system/Qt includes
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

bool SteamGameProcessLogTracker::isAnyProcessRunning(const AppId& app_id) const
{
    return m_app_id_to_process_ids.contains(app_id);
}

void SteamGameProcessLogTracker::onLogChanged(const std::vector<QString>& new_lines)
{
    std::set<AppId> added_app_ids;
    std::set<AppId> removed_app_ids;

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

            auto data_it = m_app_id_to_process_ids.find(*app_id);
            if (data_it == std::end(m_app_id_to_process_ids))
            {
                data_it = m_app_id_to_process_ids.emplace(*app_id, std::set<uint>{}).first;

                if (!removed_app_ids.contains(*app_id))
                {
                    added_app_ids.insert(*app_id);
                }
                removed_app_ids.erase(*app_id);
            }

            data_it->second.insert(pid);
            continue;
        }

        static const QRegularExpression remove_regex{
            R"((?:Game (\d+) going away.* PID (\d+))|(?:AppID (\d+) no longer.* PID (\d+)))"};
        if (const auto match{remove_regex.match(line)}; match.hasMatch())
        {
            const auto app_id{AppId::fromString(match.hasCaptured(1) ? match.captured(1) : match.captured(3))};
            if (!app_id)
            {
                qCWarning(lc::steam) << "Failed to get AppID from" << line;
                continue;
            }

            const auto pid{(match.hasCaptured(2) ? match.captured(2) : match.captured(4)).toUInt()};
            if (pid == 0)
            {
                qCWarning(lc::steam) << "Failed to get PID from" << line;
                continue;
            }

            if (!m_app_id_to_process_ids.contains(*app_id))
            {
                qCWarning(lc::steam) << "Trying to remove PID" << pid << "from" << app_id->getId()
                                     << "but AppID is not tracked!";
            }


            qCDebug(lc::steam) << "Removing PID" << pid << "from all tracked AppIDs";
            for (auto data_it = m_app_id_to_process_ids.begin(); data_it != m_app_id_to_process_ids.end(); )
            {
                data_it->second.erase(pid);
                if (data_it->second.empty())
                {
                    const auto empty_app_id = data_it->first;

                    data_it = m_app_id_to_process_ids.erase(data_it);

                    if (!added_app_ids.contains(empty_app_id))
                    {
                        removed_app_ids.insert(empty_app_id);
                    }
                    added_app_ids.erase(empty_app_id);
                }
                else
                {
                    ++data_it;
                }
            }
        }
    }

    for (const auto& app_id : added_app_ids)
    {
        qCInfo(lc::steam) << "Running processes added for AppID:" << app_id.getId();
    }

    for (const auto& app_id : removed_app_ids)
    {
        qCInfo(lc::steam) << "Running processes removed for AppID:" << app_id.getId();
    }
}
}  // namespace steam
