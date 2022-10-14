// header file include
#include "login.h"

// system/Qt includes
#include <QJsonObject>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QString     TYPE{QLatin1String("LOGIN")};
const QStringList CONTROL_TYPES{QLatin1String("STEAM"), QLatin1String("PC")};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
std::optional<Login> Login::fromJson(const QJsonDocument& data)
{
    if (data.isObject())
    {
        const auto obj_v          = data.object();
        const auto type_v         = obj_v.value(QLatin1String("type"));
        const auto id_v           = obj_v.value(QLatin1String("id"));
        const auto control_type_v = obj_v.value(QLatin1String("control_type"));

        if (type_v.isString() && type_v.toString() == TYPE)
        {
            if (id_v.isString() && control_type_v.isString())
            {
                const auto control_type_converted = control_type_v.toString();
                if (CONTROL_TYPES.contains(control_type_converted))
                {
                    return Login{id_v.toString(),
                                 control_type_converted == CONTROL_TYPES[0] ? ControlType::Steam : ControlType::Pc};
                }
            }
        }
    }
    return std::nullopt;
}
}  // namespace msgs::in