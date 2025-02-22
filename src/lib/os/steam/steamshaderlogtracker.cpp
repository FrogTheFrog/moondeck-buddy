// header file include
#include "os/steam/steamshaderlogtracker.h"

// system/Qt includes
#include <QRegularExpression>

// local includes
#include "shared/loggingcategories.h"

namespace os
{
SteamShaderLogTracker::SteamShaderLogTracker(const std::filesystem::path& logs_dir, QDateTime first_entry_time_filter)
    : SteamLogTracker(logs_dir / "shader_log.txt", logs_dir / "shader_log.previous.txt",
                      std::move(first_entry_time_filter))
{
}

bool SteamShaderLogTracker::isAppCompilingShaders(const uint app_id) const
{
    return m_apps_with_compiling_shaders.contains(app_id);
}

void SteamShaderLogTracker::onLogChanged(const std::vector<QString>& new_lines)
{
    std::map<uint, bool> new_shader_states;
    for (const QString& line : new_lines)
    {
        static const QRegularExpression regex{
            R"((?:Starting processing job for app (\d+))|(?:Destroyed compile job (\d+)))"};
        if (const auto match{regex.match(line)}; match.hasMatch())
        {
            const bool started{match.hasCaptured(1)};
            const auto app_id{(started ? match.captured(1) : match.captured(2)).toUInt()};
            if (app_id == 0)
            {
                qCWarning(lc::os) << "Failed to get AppID from" << line;
                continue;
            }

            new_shader_states[app_id] = started;
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
                qCWarning(lc::os) << "Compiling shaders for AppID:" << app_id;
            }
        }
        else
        {
            if (data_it != std::end(m_apps_with_compiling_shaders))
            {
                m_apps_with_compiling_shaders.erase(data_it);
                qCWarning(lc::os) << "Stopped compiling shaders for AppID:" << app_id;
            }
        }
    }
}
}  // namespace os
