// header file include
#include "launchapp.h"

// system/Qt includes
#include <QJsonObject>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QString TYPE{QLatin1String("LAUNCH_APP")};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
std::optional<LaunchApp> LaunchApp::fromJson(const QJsonDocument& data)
{
    if (data.isObject())
    {
        const auto obj_v    = data.object();
        const auto type_v   = obj_v.value(QLatin1String("type"));
        const auto app_id_v = obj_v.value(QLatin1String("app_id"));

        if (type_v.isString() && type_v.toString() == TYPE)
        {
            if (app_id_v.isDouble() && app_id_v.toInt(-1) > 0)
            {
                return LaunchApp{static_cast<uint>(app_id_v.toInt())};
            }
        }
    }
    return std::nullopt;
}
}  // namespace msgs::in