#pragma once

// system/Qt includes
#include <set>

// local includes
#include "appid.h"
#include "steamlogtracker.h"

namespace steam
{
class SteamShaderLogTracker : public SteamLogTracker
{
    Q_OBJECT

public:
    explicit SteamShaderLogTracker(const std::filesystem::path& logs_dir, QDateTime first_entry_time_filter);
    ~SteamShaderLogTracker() override;

    bool isAppCompilingShaders(const AppId& app_id) const;

protected:
    void onLogChanged(const std::vector<LogLine>& new_lines) override;

private:
    std::set<AppId> m_apps_with_compiling_shaders;
};
}  // namespace steam
