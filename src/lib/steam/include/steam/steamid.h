#pragma once

// system/Qt includes
#include <QObject>

namespace steam
{
class SteamId final
{
    Q_GADGET

public:
    enum class Universe
    {
        Individual,
        Public,
        Beta,
        Internal,
        Dev,
        RC
    };
    Q_ENUM(Universe)

    enum class Type
    {
        Invalid,
        Individual,
        Multiseat,
        GameServer,
        AnonGameServer,
        Pending,
        ContentServer,
        Clan,
        Chat,
        P2PSuperSeeder,
        AnonUser
    };
    Q_ENUM(Type)

    static std::optional<SteamId> fromString(const QString& str);

    QString toSteamId() const;
    QString toSteam3Id() const;

    std::uint32_t toSteamId32Uint() const;
    QString       toSteamId32() const;

    std::uint64_t toSteamId64Uint() const;
    QString       toSteamId64() const;

    QString toString() const;

    bool isNull() const;

    bool operator==(const SteamId&) const = default;
    bool operator!=(const SteamId&) const = default;

private:
    static std::optional<SteamId> tryConstruct(const QString& original_str, const std::optional<Universe>& universe,
                                               const std::optional<Type>& type, std::uint32_t account_instance,
                                               std::uint32_t account_number, std::uint32_t parity_bit);

    explicit SteamId(const Universe& universe, const Type& type, std::uint32_t account_instance,
                     std::uint32_t account_number, bool parity_bit);

    Universe      m_universe;
    Type          m_type;
    std::uint32_t m_account_instance;
    std::uint32_t m_account_number;
    bool          m_parity_bit;
};
}  // namespace steam