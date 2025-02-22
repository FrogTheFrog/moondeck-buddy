#pragma once

// system/Qt includes
#include <set>

// local includes
#include "steamlogtracker.h"

namespace os
{
class SteamGameProcessLogTracker : public SteamLogTracker
{
    Q_OBJECT

public:
    explicit SteamGameProcessLogTracker(const std::filesystem::path& logs_dir, QDateTime first_entry_time_filter);
    ~SteamGameProcessLogTracker() override = default;

    bool isAnyProcessRunning(uint app_id) const;

protected:
    void onLogChanged(const std::vector<QString>& new_lines) override;

private:
    std::map<uint, std::set<uint>> m_app_id_to_process_ids;
};
}  // namespace os
