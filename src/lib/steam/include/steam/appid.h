#pragma once

// system/Qt includes
#include <QObject>

namespace steam
{
class AppId final
{
    Q_GADGET

public:
    enum class IdType
    {
        SteamApp,
        NonSteamApp,
        ModGameId,
        ShortcutGameId
    };
    Q_ENUM(IdType);

    explicit AppId(std::uint64_t app_or_game_id);

    static std::optional<AppId> fromString(const QString& app_id);

    std::uint64_t getId() const;
    IdType        getIdType() const;

    bool          isGameId() const;
    std::uint64_t getGameId() const;

    auto operator<=>(const AppId&) const = default;

private:
    std::uint64_t m_id;
    IdType        m_id_type;
};
}  // namespace steam