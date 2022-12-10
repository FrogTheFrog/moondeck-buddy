#pragma once

// system/Qt includes
#include <QJsonDocument>

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
struct SteamStatus
{
    static std::optional<SteamStatus> fromJson(const QJsonDocument& data);
};
}  // namespace msgs::in
