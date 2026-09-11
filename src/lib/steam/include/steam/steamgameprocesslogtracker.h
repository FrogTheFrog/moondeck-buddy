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
    using PidDataMap        = QMap<uint, QDateTime>;
    using AppIdToPidDataMap = std::map<AppId, PidDataMap>;

    explicit SteamGameProcessLogTracker(const std::filesystem::path& logs_dir, QDateTime first_entry_time_filter);
    ~SteamGameProcessLogTracker() override;

    const AppIdToPidDataMap& getAppIdData() const;

protected:
    void onLogChanged(const std::vector<LogLine>& new_lines) override;

private:
    AppIdToPidDataMap m_app_id_to_process_ids;
};
}  // namespace steam
