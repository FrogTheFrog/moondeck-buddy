#pragma once

// local includes
#include "steamlogtracker.h"

namespace os
{
class SteamContentLogTracker : public SteamLogTracker
{
    Q_OBJECT

public:
    enum class State
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
        ComponentInUse
    };

    explicit SteamContentLogTracker(const std::filesystem::path& logs_dir, QDateTime first_entry_time_filter);
    ~SteamContentLogTracker() override = default;

protected:
    void onLogChanged(const std::vector<QString>& new_lines) override;
};
}  // namespace os
