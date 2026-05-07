#pragma once

// local includes
#include "common/enums.h"
#include "steamlogtracker.h"

namespace steam
{
class SteamWebHelperLogTracker : public SteamLogTracker
{
    Q_OBJECT

public:
    explicit SteamWebHelperLogTracker(const std::filesystem::path& logs_dir, QDateTime first_entry_time_filter);
    ~SteamWebHelperLogTracker() override = default;

    enums::SteamUiMode getSteamUiMode() const;

protected:
    void onLogChanged(const std::vector<QString>& new_lines) override;

private:
    enums::SteamUiMode m_ui_mode{enums::SteamUiMode::Unknown};
};
}  // namespace steam
