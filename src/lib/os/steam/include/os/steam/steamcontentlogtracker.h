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
        UpdateOptional,
        FullyInstalled,
        AppRunning,
        FilesMissing,
        Uninstalling,
        Uninstalled,
        ComponentInUse,
        Terminating,
        PrefetchingInfo
    };
    Q_ENUM(AppStateChange)

    explicit SteamContentLogTracker(const std::filesystem::path& logs_dir, QDateTime first_entry_time_filter);
    ~SteamContentLogTracker() override = default;

    AppState getAppState(std::uint64_t app_id) const;

protected:
    void onLogChanged(const std::vector<QString>& new_lines) override;

private:
    std::map<std::uint64_t, AppState> m_app_states;
};
}  // namespace os
