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

namespace msgs::out
{
QJsonDocument SteamStatus::toJson(const SteamStatus& data)
{
    QJsonObject obj;

    obj["type"]             = TYPE;
    obj["app_id"]           = static_cast<int>(data.m_app_id);
    obj["steam_is_running"] = data.m_steam_is_running;

    return QJsonDocument{obj};
}
}  // namespace msgs::out