#pragma once

// system/Qt includes
#include <QJsonDocument>

// local includes
#include "shared/enums.h"

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::out
{
// NOLINTNEXTLINE(*-member-init)
struct SteamStatus
{
    static QJsonDocument toJson(const SteamStatus& data);

    uint                m_running_app_id;
    std::optional<uint> m_last_launched_app_is_updating;
    bool                m_steam_is_running;
    shared::StreamState m_stream_state;
};
}  // namespace msgs::out
