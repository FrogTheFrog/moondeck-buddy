#pragma once

// local includes
#include "steamlogtracker.h"

namespace os
{
class SteamWebHelperLogTracker : public SteamLogTracker
{
    Q_OBJECT

public:
    enum class UiMode
    {
        Unknown,
        FinishedLoading, // TODO
        Desktop,
        BigPicture
    };
    Q_ENUM(UiMode)

    explicit SteamWebHelperLogTracker(const std::filesystem::path& logs_dir, QDateTime first_entry_time_filter);
    ~SteamWebHelperLogTracker() override = default;

    UiMode getUiMode() const;

protected:
    void onLogChanged(const std::vector<QString>& new_lines) override;

private:
    UiMode m_ui_mode{UiMode::Unknown};
};
}  // namespace os
