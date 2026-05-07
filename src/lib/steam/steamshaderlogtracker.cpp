// header file include
#include "steam/steamshaderlogtracker.h"

// system/Qt includes
#include <QRegularExpression>

// local includes
#include "common/loggingcategories.h"

namespace steam
{
SteamShaderLogTracker::SteamShaderLogTracker(const std::filesystem::path& logs_dir, QDateTime first_entry_time_filter)
    : SteamLogTracker(logs_dir / "shader_log.txt", logs_dir / "shader_log.previous.txt",
                      std::move(first_entry_time_filter))
{
}

bool SteamShaderLogTracker::isAppCompilingShaders(const AppId& app_id) const
{
    return m_apps_with_compiling_shaders.contains(app_id);
}

void SteamShaderLogTracker::onLogChanged(const std::vector<QString>& new_lines)
{
    std::map<AppId, bool> new_shader_states;
    for (const QString& line : new_lines)
    {
        static const QRegularExpression regex{
            R"((?:Starting processing job for app (\d+))|(?:Destroyed compile job (\d+)))"};
        if (const auto match{regex.match(line)}; match.hasMatch())
        {
            const bool started{match.hasCaptured(1)};
            const auto app_id{AppId::fromString(started ? match.captured(1) : match.captured(2))};
            if (!app_id)
            {
                qCWarning(lc::steam) << "Failed to get AppID from" << line;
                continue;
            }

            new_shader_states[*app_id] = started;
        }
    }

    for (const auto [app_id, state] : new_shader_states)
    {
        auto data_it{m_apps_with_compiling_shaders.find(app_id)};
        if (state)
        {
            if (data_it == std::end(m_apps_with_compiling_shaders))
            {
                m_apps_with_compiling_shaders.insert(app_id);
                qCInfo(lc::steam) << "Compiling shaders for AppID:" << app_id.getId();
            }
        }
        else
        {
            if (data_it != std::end(m_apps_with_compiling_shaders))
            {
                m_apps_with_compiling_shaders.erase(data_it);
                qCInfo(lc::steam) << "Stopped compiling shaders for AppID:" << app_id.getId();
            }
        }
    }
}
}  // namespace steam
