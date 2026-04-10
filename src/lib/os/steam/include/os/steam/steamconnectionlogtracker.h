#pragma once

// system/Qt includes
#include <optional>

// local includes
#include "shared/steamid.h"
#include "steamlogtracker.h"

namespace os
{
class SteamConnectionLogTracker : public SteamLogTracker
{
    Q_OBJECT

public:
    explicit SteamConnectionLogTracker(const std::filesystem::path& logs_dir, QDateTime first_entry_time_filter);
    ~SteamConnectionLogTracker() override = default;

    const std::optional<shared::SteamId>& getCurrentSteamId() const;

protected:
    void onLogChanged(const std::vector<QString>& new_lines) override;

private:
    std::optional<shared::SteamId> m_current_steam_id;
};
}  // namespace os
