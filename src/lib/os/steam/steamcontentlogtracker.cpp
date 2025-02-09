// header file include
#include "os/steam/steamcontentlogtracker.h"

// system/Qt includes
#include <QRegularExpression>

// local includes
#include "shared/loggingcategories.h"

namespace os
{
namespace
{
std::map<uint, SteamContentLogTracker::AppState>
    remapStateChanges(const std::map<uint, QVector<SteamContentLogTracker::AppStateChange>>& input)
{
    using AppStateChange = SteamContentLogTracker::AppStateChange;
    using AppState       = SteamContentLogTracker::AppState;

    const auto map_state{[](const AppStateChange state_change)
                         {
                             switch (state_change)
                             {
                                 case AppStateChange::UpdateRequired:
                                 case AppStateChange::UpdateQueued:
                                 case AppStateChange::UpdateRunning:
                                 case AppStateChange::UpdateStarted:
                                     return AppState::Updating;
                                 case AppStateChange::AppRunning:
                                     return AppState::Running;
                                 case AppStateChange::FullyInstalled:
                                 case AppStateChange::FilesMissing:
                                 case AppStateChange::Uninstalling:
                                 case AppStateChange::Uninstalled:
                                 case AppStateChange::ComponentInUse:
                                 case AppStateChange::Terminating:
                                     return AppState::Stopped;
                             }

                             Q_UNREACHABLE();
                         }};

    std::map<uint, AppState> new_app_states;
    for (const auto& [app_id, state_changes] : input)
    {
        qCDebug(lc::os) << "New state changes for AppID" << app_id << "detected:" << state_changes;

        AppState new_state{AppState::Stopped};
        for (const auto state_change : state_changes)
        {
            const auto state{map_state(state_change)};
            if (state != AppState::Stopped)
            {
                new_state = state;
                if (new_state == AppState::Running)
                {
                    break;
                }
            }
        }

        new_app_states[app_id] = new_state;
    }
    return new_app_states;
}
}  // namespace

SteamContentLogTracker::SteamContentLogTracker(const std::filesystem::path& logs_dir, QDateTime first_entry_time_filter)
    : SteamLogTracker(logs_dir / "content_log.txt", logs_dir / "content_log.previous.txt",
                      std::move(first_entry_time_filter))
{
}

SteamContentLogTracker::AppState SteamContentLogTracker::getAppState(const uint app_id) const
{
    const auto it = m_app_states.find(app_id);
    return it != std::end(m_app_states) ? it->second : AppState::Stopped;
}

void SteamContentLogTracker::onLogChanged(const std::vector<QString>& new_lines)
{
    static const auto known_states{
        []()
        {
            const auto                        enum_size{QMetaEnum::fromType<AppStateChange>().keyCount()};
            std::map<QString, AppStateChange> states;
            for (int i = 0; i < enum_size; ++i)
            {
                const auto value{static_cast<AppStateChange>(QMetaEnum::fromType<AppStateChange>().value(i))};
                QString    key{QMetaEnum::fromType<AppStateChange>().key(i)};
                key = key.replace(QRegularExpression{R"(([A-Z]))"}, R"( \1)").trimmed();

                states[key] = value;
            }

            return states;
        }()};

    std::map<uint, QVector<AppStateChange>> new_change_states;
    for (const QString& line : new_lines)
    {
        static const QRegularExpression mode_regex{R"(AppID\s(\d+)\sstate\schanged\s:\s(.*),)"};
        const auto                      match{mode_regex.match(line)};
        if (match.hasMatch())
        {
            const auto app_id{match.captured(1).toUInt()};
            if (app_id == 0)
            {
                qCWarning(lc::os) << "Failed to get AppID from" << line;
                continue;
            }

            QVector<AppStateChange> mapped_change_states;
            const auto              change_states{match.captured(2).split(',')};
            for (const auto& state : change_states)
            {
                auto it = known_states.find(state);
                if (it == std::end(known_states))
                {
                    qCWarning(lc::os) << "Unmapped state for AppID" << app_id << "found:" << state;
                    continue;
                }

                mapped_change_states.append(it->second);
            }

            new_change_states[app_id] = std::move(mapped_change_states);
        }
    }

    const auto new_app_states = remapStateChanges(new_change_states);
    for (const auto [app_id, app_state] : new_app_states)
    {
        auto it = m_app_states.find(app_id);
        if (it == std::end(m_app_states))
        {
            if (app_state == AppState::Stopped)
            {
                continue;
            }

            qCInfo(lc::os) << "New app state for AppID" << app_id << "detected:" << lc::qEnumToString(AppState::Stopped)
                           << "->" << lc::qEnumToString(app_state);
            m_app_states[app_id] = app_state;
            continue;
        }

        if (it->second == app_state)
        {
            continue;
        }

        qCInfo(lc::os) << "New app state for AppID" << app_id << "detected:" << lc::qEnumToString(it->second) << "->"
                       << lc::qEnumToString(app_state);
        if (app_state == AppState::Stopped)
        {
            m_app_states.erase(it);
        }
        else
        {
            it->second = app_state;
        }
    }
}
}  // namespace os
