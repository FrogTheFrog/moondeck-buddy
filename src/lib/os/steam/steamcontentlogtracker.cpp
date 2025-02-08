// header file include
#include "os/steam/steamcontentlogtracker.h"

// system/Qt includes
#include <QRegularExpression>

// local includes
#include "shared/loggingcategories.h"

namespace os
{
SteamContentLogTracker::SteamContentLogTracker(const std::filesystem::path& logs_dir, QDateTime first_entry_time_filter)
    : SteamLogTracker(logs_dir / "webhelper.txt", logs_dir / "webhelper.previous.txt",
                      std::move(first_entry_time_filter))
{
}

void SteamContentLogTracker::onLogChanged(const std::vector<QString>& new_lines)
{
    for (const QString& line : new_lines)
    {
        static const QRegularExpression mode_regex{R"(AppID\s(\d+)\sstate\schanged\s:\s(.+),)"};
        const auto                      match{mode_regex.match(line)};
        if (match.hasMatch())
        {
           // new_ui_mode = match.hasCaptured(1) ? UiMode::Desktop : UiMode::BigPicture;
        }
    }
}
}  // namespace os
