#pragma once

// system/Qt includes
#include <QJsonDocument>

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::out
{
struct SteamStatus
{
    static QJsonDocument toJson(const SteamStatus& data);

    uint m_app_id;
    bool m_steam_is_running;
};
}  // namespace msgs::out
