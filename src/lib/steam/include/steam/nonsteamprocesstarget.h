#pragma once
#include "steam/shortcutsvdf.h"
#include <QDateTime>
#include <QString>
#include <map>
#include <optional>

namespace steam
{
struct NonSteamProcessTarget
{
    enum class Kind
    {
        Executable,
        Directory,
        Package
    };
    Kind                                        m_kind;
    QString                                     m_value;
    static std::optional<NonSteamProcessTarget> fromShortcut(const ShortcutsVdfEntry& shortcut);
    std::map<uint, QDateTime>                   getProcesses() const;
};
}  // namespace steam
