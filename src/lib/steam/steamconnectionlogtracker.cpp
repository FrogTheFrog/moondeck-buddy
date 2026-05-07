// header file include
#include "steam/steamconnectionlogtracker.h"

// system/Qt includes
#include <QRegularExpression>

// local includes
#include "common/enums.h"
#include "common/loggingcategories.h"

namespace steam
{
SteamConnectionLogTracker::SteamConnectionLogTracker(const std::filesystem::path& logs_dir,
                                                     QDateTime                    first_entry_time_filter)
    : SteamLogTracker(logs_dir / "connection_log.txt", logs_dir / "connection_log.previous.txt",
                      std::move(first_entry_time_filter))
{
}

const std::optional<SteamId>& SteamConnectionLogTracker::getCurrentSteamId() const
{
    return m_current_steam_id;
}

void SteamConnectionLogTracker::onLogChanged(const std::vector<QString>& new_lines)
{
    std::optional<SteamId> new_steam_id;
    for (const QString& line : new_lines)
    {
        static const QRegularExpression regex{R"(^(?:\[[^\]]*\]\s*){2}\[([^\]]+)\])"};
        if (const auto match{regex.match(line)}; match.hasMatch())
        {
            new_steam_id = SteamId::fromString(match.captured(1));
        }
    }

    if (new_steam_id && m_current_steam_id != new_steam_id)
    {
        if (new_steam_id->isNull())
        {
            qCInfo(lc::steam).noquote().nospace() << "New user SteamId detected: NULL";
            m_current_steam_id = std::nullopt;
        }
        else
        {
            qCInfo(lc::steam).noquote().nospace() << "New user SteamId detected:\n" << new_steam_id->toString();
            m_current_steam_id = new_steam_id;
        }
    }
}
}  // namespace steam
