#pragma once

// system/Qt includes
#include <set>

// local includes
#include "shared/appid.h"
#include "steamlogtracker.h"

namespace os
{
class SteamShaderLogTracker : public SteamLogTracker
{
    Q_OBJECT

public:
    explicit SteamShaderLogTracker(const std::filesystem::path& logs_dir, QDateTime first_entry_time_filter);
    ~SteamShaderLogTracker() override = default;

    bool isAppCompilingShaders(const shared::AppId& app_id) const;

protected:
    void onLogChanged(const std::vector<QString>& new_lines) override;

private:
    std::set<shared::AppId> m_apps_with_compiling_shaders;
};
}  // namespace os
