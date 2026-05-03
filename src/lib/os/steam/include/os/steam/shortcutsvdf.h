#pragma once

// system/Qt includes
#include <filesystem>

// local includes
#include "shared/appid.h"
#include "shared/steamid.h"

namespace os
{
// This is a very "son, we have a parser at home" kind of parser, very basic, but gets the job done...
struct ShortcutsVdfEntry
{
    shared::AppId m_app_id;
    QString       m_app_name;
    QString       m_start_dir;

    static std::optional<std::vector<ShortcutsVdfEntry>> scrapeShortcutsVdf(const QByteArray& contents);
    static std::optional<std::vector<ShortcutsVdfEntry>> scrapeShortcutsVdf(const std::filesystem::path& steam_dir,
                                                                            const shared::SteamId&       user_id);
};
}  // namespace os
