// header file include
#include "steamstatus.h"

// system/Qt includes
#include <QJsonObject>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QString TYPE{QLatin1String("STEAM_STATUS")};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
std::optional<SteamStatus> SteamStatus::fromJson(const QJsonDocument& data)
{
    if (data.isObject())
    {
        const auto obj_v  = data.object();
        const auto type_v = obj_v.value(QLatin1String("type"));

        if (type_v.isString() && type_v.toString() == TYPE)
        {
            return SteamStatus{};
        }
    }
    return std::nullopt;
}
}  // namespace msgs::in