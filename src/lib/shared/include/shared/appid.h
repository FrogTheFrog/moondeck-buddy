#pragma once

// system/Qt includes
#include <QString>

namespace shared
{
class AppId final
{
public:
    explicit AppId(std::uint64_t app_or_game_id);

    static std::optional<AppId> fromString(const QString& app_id);

    std::uint64_t getId() const;
    bool          isGameId() const;
    AppId         toGameId() const;

    auto operator<=>(const AppId&) const = default;

private:
    std::uint64_t m_id;
};
}  // namespace shared