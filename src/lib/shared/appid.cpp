// class header include
#include "shared/appid.h"

namespace
{
bool hasGameIdMarker(const std::uint64_t app_or_game_id)
{
    return (app_or_game_id & 0x02000000U) != 0U;
}
}  // namespace

namespace shared
{
AppId::AppId(const std::uint64_t app_or_game_id)
    : m_id{app_or_game_id}
{
}

std::optional<AppId> AppId::fromString(const QString& app_id)
{
    static_assert(sizeof(qulonglong) == sizeof(std::uint64_t));
    static_assert(sizeof(qlonglong) == sizeof(std::int64_t));

    bool          success{false};
    std::uint64_t app_or_game_id{app_id.toULongLong(&success)};
    if (!success)
    {
        const std::int64_t signed_app_or_game_id{app_id.toLongLong(&success)};
        if (!success)
        {
            return std::nullopt;
        }

        app_or_game_id = static_cast<std::uint64_t>(signed_app_or_game_id);
    }

    return AppId{app_or_game_id};
}

std::uint64_t AppId::getId() const
{
    return m_id;
}

bool AppId::isGameId() const
{
    return hasGameIdMarker(getId());
}

AppId AppId::toGameId() const
{
    return AppId{isGameId() ? getId() : getId() << 32U | 0x02000000U};
}
}  // namespace shared