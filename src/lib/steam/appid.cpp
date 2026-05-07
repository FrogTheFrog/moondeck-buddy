// class header include
#include "steam/appid.h"

namespace
{
constexpr std::uint32_t internal_app_id_mask{0x00FFFFFF};
constexpr std::uint32_t internal_app_id_offset{24U};
constexpr std::uint32_t high_bits_offset{32U};
constexpr std::uint32_t game_id_type_mask{0xFF};
constexpr std::uint32_t mod_id_flag{0x80000000};
constexpr std::uint32_t game_mod_game_id_type{1};
constexpr std::uint32_t shortcut_game_id_type{2};

std::uint32_t getLowBits(const std::uint64_t value)
{
    return static_cast<std::uint32_t>(value);
}

std::uint32_t getHighBits(const std::uint64_t value)
{
    return static_cast<std::uint32_t>(value >> high_bits_offset);
}

std::uint32_t getAppId(const std::uint64_t app_or_game_id)
{
    return getLowBits(app_or_game_id) & internal_app_id_mask;
}

std::uint32_t getModId(const std::uint64_t app_or_game_id)
{
    const auto bits{getHighBits(app_or_game_id)};
    return (bits & mod_id_flag) != 0U ? bits : 0U;
}

std::uint32_t getTypeValue(const std::uint64_t value)
{
    return getLowBits(value) >> internal_app_id_offset & game_id_type_mask;
}

std::uint64_t toShortcutGameId(const std::uint64_t non_steam_app_id)
{
    return (non_steam_app_id << high_bits_offset) | (shortcut_game_id_type << internal_app_id_offset);
}

std::optional<steam::AppId::IdType> guessGameId(const std::uint64_t app_or_game_id)
{
    switch (getTypeValue(app_or_game_id))
    {
        case game_mod_game_id_type:
            if (getAppId(app_or_game_id) != 0U && getModId(app_or_game_id) != 0U)
            {
                return steam::AppId::IdType::ModGameId;
            }
            break;
        case shortcut_game_id_type:
            if (getModId(app_or_game_id) != 0U)
            {
                return steam::AppId::IdType::ShortcutGameId;
            }
            break;
        default:
            // This is very likely not a game id value
            break;
    }

    return std::nullopt;
}

steam::AppId::IdType guessAppIdType(const std::uint64_t app_or_game_id)
{
    if (const auto game_id_type{guessGameId(app_or_game_id)}; game_id_type)
    {
        return *game_id_type;
    }

    // Technically we should also handle the mod app id type, but we will be assuming/supporting non-Steam games as
    // you would need to otherwise provide the type explicitly, which we don't know :/
    return app_or_game_id == getAppId(app_or_game_id) ? steam::AppId::IdType::SteamApp
                                                      : steam::AppId::IdType::NonSteamApp;
}
}  // namespace

namespace steam
{
AppId::AppId(const std::uint64_t app_or_game_id)
    : m_id{app_or_game_id}
    , m_id_type{guessAppIdType(app_or_game_id)}
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

AppId::IdType AppId::getIdType() const
{
    return m_id_type;
}

bool AppId::isGameId() const
{
    using enum IdType;

    switch (m_id_type)
    {
        case SteamApp:
            return false;
        case NonSteamApp:
            return false;
        case ModGameId:
            return true;
        case ShortcutGameId:
            return true;
    }

    Q_UNREACHABLE();
}

std::uint64_t AppId::getGameId() const
{
    using enum IdType;

    switch (m_id_type)
    {
        case SteamApp:
            // GameId for a Steam app is the same as AppId
            return m_id;
        case NonSteamApp:
            // See comment in guessAppIdType
            return ::toShortcutGameId(m_id);
        case ModGameId:
            return m_id;
        case ShortcutGameId:
            return m_id;
    }

    Q_UNREACHABLE();
}
}  // namespace steam