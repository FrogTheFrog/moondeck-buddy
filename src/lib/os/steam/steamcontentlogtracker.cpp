// header file include
#include "os/steam/steamcontentlogtracker.h"

// system/Qt includes
#include <QRegularExpression>

// local includes
#include "shared/loggingcategories.h"

namespace os
{
SteamContentLogTracker::SteamContentLogTracker(const std::filesystem::path& logs_dir)
    : SteamLogTracker(logs_dir / "content_log.txt", logs_dir / "content_log.txt.backup")
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
