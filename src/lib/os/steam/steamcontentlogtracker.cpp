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
        qDebug() << line;
    }
}
}  // namespace os
