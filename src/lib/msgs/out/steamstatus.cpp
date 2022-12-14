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

    obj["type"]                          = TYPE;
    obj["running_app_id"]                = static_cast<int>(data.m_running_app_id);
    obj["last_launched_app_is_updating"] = data.m_last_launched_app_is_updating
                                               ? QJsonValue{static_cast<int>(*data.m_last_launched_app_is_updating)}
                                               : QJsonValue{QJsonValue::Null};
    obj["steam_is_running"]              = data.m_steam_is_running;
    obj["stream_state"]                  = QVariant::fromValue(data.m_stream_state).toString();

    return QJsonDocument{obj};
}
}  // namespace msgs::out