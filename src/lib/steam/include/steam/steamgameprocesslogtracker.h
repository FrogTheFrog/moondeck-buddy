#pragma once

// local includes
#include "appid.h"
#include "steamlogtracker.h"

namespace steam
{
class SteamGameProcessLogTracker : public SteamLogTracker
{
    Q_OBJECT

public:
    explicit SteamGameProcessLogTracker(const std::filesystem::path& logs_dir, QDateTime first_entry_time_filter);
    ~SteamGameProcessLogTracker() override = default;

    bool isAnyProcessRunning(const AppId& app_id) const;

protected:
    void onLogChanged(const std::vector<QString>& new_lines) override;

private:
    std::map<AppId, QSet<uint>> m_app_id_to_process_ids;
};
}  // namespace steam
