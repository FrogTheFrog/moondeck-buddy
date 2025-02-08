#pragma once

// local includes
#include "steamlogtracker.h"

namespace os
{
class SteamContentLogTracker : public SteamLogTracker
{
    Q_OBJECT

public:
    enum class AppState
    {
        Stopped,
        Running,
        Updating
    };
    Q_ENUM(AppState)

    enum class AppStateChange
    {
        UpdateRequired,
        UpdateQueued,
        UpdateRunning,
        UpdateStarted,
        FullyInstalled,
        AppRunning,
        FilesMissing,
        Uninstalling,
        Uninstalled,
        ComponentInUse,
        Terminating
    };
    Q_ENUM(AppStateChange)

    explicit SteamContentLogTracker(const std::filesystem::path& logs_dir, QDateTime first_entry_time_filter);
    ~SteamContentLogTracker() override = default;

    AppState getAppState(uint app_id) const;

protected:
    void onLogChanged(const std::vector<QString>& new_lines) override;

private:
    std::map<uint, AppState> m_app_states;
};
}  // namespace os
