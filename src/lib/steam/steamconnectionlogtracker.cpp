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

SteamConnectionLogTracker::~SteamConnectionLogTracker()
{
    if (m_current_steam_id)
    {
        m_current_steam_id = std::nullopt;
        emit signalStateChanged();
        emit signalSteamCurrentUserChanged();
    }
}

const std::optional<SteamId>& SteamConnectionLogTracker::getCurrentSteamId() const
{
    return m_current_steam_id;
}

void SteamConnectionLogTracker::onLogChanged(const std::vector<LogLine>& new_lines)
{
    std::optional<SteamId> new_steam_id;
    for (const auto& line : new_lines)
    {
        static const QRegularExpression regex{R"(^(?:\[[^\]]*\]\s*){2}\[([^\]]+)\])"};
        if (const auto match{regex.match(line.m_text)}; match.hasMatch())
        {
            new_steam_id = SteamId::fromString(match.captured(1));
        }
    }

    if (new_steam_id && m_current_steam_id != new_steam_id)
    {
        qCInfo(lc::steam).noquote().nospace() << "User SteamId changed:\n" << new_steam_id->toString();
        m_current_steam_id = new_steam_id;
        emit signalStateChanged();
        emit signalSteamCurrentUserChanged();
    }
}
}  // namespace steam
