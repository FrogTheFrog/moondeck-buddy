// class header include
#include "shared/steamid.h"

// system/Qt includes
#include <QRegularExpression>

// local includes
#include "shared/enums.h"
#include "shared/loggingcategories.h"

namespace
{
std::optional<std::uint64_t> stringToUint64(const QString& str)
{
    static_assert(sizeof(qulonglong) == sizeof(std::uint64_t));
    bool                converted{false};
    const std::uint64_t number{str.toULongLong(&converted)};

    if (converted)
    {
        return number;
    }

    return std::nullopt;
}

const QMap<shared::SteamId::Type, std::vector<QString>>& getTypeLetterMap()
{
    using enum shared::SteamId::Type;
    static const QMap<shared::SteamId::Type, std::vector<QString>> type_letters{
        {Invalid, {"I", "i"}},   {Individual, {"U"}},    {Multiseat, {"M"}},     {GameServer, {"G"}},
        {AnonGameServer, {"A"}}, {Pending, {"P"}},       {ContentServer, {"C"}}, {Clan, {"g"}},
        {Chat, {"T", "L", "c"}}, {P2PSuperSeeder, {""}}, {AnonUser, {"a"}}};
    return type_letters;
}

QString getTypeLetter(const shared::SteamId::Type type)
{
    return getTypeLetterMap().value(type, {"I"}).at(0);
}

std::optional<shared::SteamId::Type> getTypeFromLetter(const QString& letter_to_find)
{
    for (const auto& [type, letters] : getTypeLetterMap().asKeyValueRange())
    {
        if (std::ranges::any_of(letters, [&letter_to_find](const auto& value) { return letter_to_find == value; }))
        {
            return type;
        }
    }

    return std::nullopt;
}
}  // namespace

namespace shared
{
std::optional<SteamId> SteamId::fromString(const QString& str)
{
    // See https://developer.valvesoftware.com/wiki/SteamID

    if (const auto number{stringToUint64(str)})
    {
        const auto parity_bit{static_cast<std::uint32_t>(*number & 0x0000000000000001U)};
        const auto account_number{static_cast<std::uint32_t>((*number & 0x00000000FFFFFFFEU) >> 1U)};
        const auto account_instance{static_cast<std::uint32_t>((*number & 0x000FFFFF00000000U) >> 32U)};
        const auto type{static_cast<std::uint32_t>((*number & 0x00F0000000000000U) >> 52U)};
        const auto universe{static_cast<std::uint32_t>((*number & 0xFF00000000000000U) >> 56U)};

        return tryConstruct(str, enums::qEnumFromUInt<Universe>(universe), enums::qEnumFromUInt<Type>(type),
                            account_instance, account_number, parity_bit);
    }

    static const QRegularExpression steam_3_id_regex{R"(\[?(\w):(\d):(\d+)\]?)"};
    if (const auto match{steam_3_id_regex.match(str)}; match.hasMatch())
    {
        const auto type{getTypeFromLetter(match.captured(1).trimmed())};
        const auto universe{enums::qEnumFromIntString<Universe>(match.captured(2).trimmed())};

        bool                converted{false};
        const std::uint32_t steam32_id{match.captured(3).trimmed().toUInt(&converted)};
        if (!converted)
        {
            qCWarning(lc::shared()) << "Could parse Steam32 ID from" << str;
            return std::nullopt;
        }

        const std::uint32_t     parity_bit{steam32_id & 0x00000001U};
        const std::uint32_t     account_number{(steam32_id & 0xFFFFFFFEU) / 2U};
        constexpr std::uint32_t account_instance{1 /* We are forced to guess :/ */};

        return tryConstruct(str, universe, type, account_instance, account_number, parity_bit);
    }

    return std::nullopt;
}

QString SteamId::toSteamId() const
{
    QString buffer;
    {
        QTextStream stream{&buffer};
        stream << "STEAM_" << static_cast<int>(m_universe) << ":" << static_cast<int>(m_parity_bit) << ":"
               << static_cast<int>(m_account_number);
    }

    return buffer;
}

QString SteamId::toSteam3Id() const
{
    QString buffer;
    {
        QTextStream stream{&buffer};
        stream << getTypeLetter(m_type) << ":" << static_cast<int>(m_universe) << ":" << toSteamId32();
    }

    return buffer;
}

std::uint32_t SteamId::toSteamId32Uint() const
{
    return (m_account_number * 2U) + (m_parity_bit ? 1U : 0U);
}

QString SteamId::toSteamId32() const
{
    return QString::number(toSteamId32Uint());
}

std::uint64_t SteamId::toSteamId64Uint() const
{
    const auto parity_bit{static_cast<std::uint64_t>(m_parity_bit ? 0x1U : 0x0U)};
    const auto account_number{static_cast<std::uint64_t>(m_account_number) << 1U};
    const auto account_instance{static_cast<std::uint64_t>(m_account_instance) << 32U};
    const auto type{static_cast<std::uint64_t>(m_type) << 52U};
    const auto universe{static_cast<std::uint64_t>(m_universe) << 56U};

    return universe | type | account_number | account_instance | parity_bit;
}

QString SteamId::toSteamId64() const
{
    return QString::number(toSteamId64Uint());
}

QString SteamId::toString() const
{
    QString buffer;
    {
        QTextStream stream{&buffer};
        stream << "Steam ID  : " << toSteamId() << "\n";
        stream << "Steam3 ID : " << toSteam3Id() << "\n";
        stream << "Steam32 ID: " << toSteamId32() << "\n";
        stream << "Steam64 ID: " << toSteamId64();
    }

    return buffer;
}

std::optional<SteamId> SteamId::tryConstruct(const QString& original_str, const std::optional<Universe>& universe,
                                             const std::optional<Type>& type, std::uint32_t account_instance,
                                             std::uint32_t account_number, std::uint32_t parity_bit)
{
    if (!universe)
    {
        qCWarning(lc::shared()) << "Could parse SteamID Universe from" << original_str;
        return std::nullopt;
    }

    if (!type)
    {
        qCWarning(lc::shared()) << "Could parse SteamID Type from" << original_str;
        return std::nullopt;
    }

    return SteamId{*universe, *type, account_instance, account_number, static_cast<bool>(parity_bit)};
}

SteamId::SteamId(const Universe& universe, const Type& type, const std::uint32_t account_instance,
                 const std::uint32_t account_number, const bool parity_bit)
    : m_universe{universe}
    , m_type{type}
    , m_account_instance{account_instance}
    , m_account_number{account_number}
    , m_parity_bit{parity_bit}
{
}

}  // namespace shared