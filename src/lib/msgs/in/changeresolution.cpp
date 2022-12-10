// header file include
#include "changeresolution.h"

// system/Qt includes
#include <QJsonObject>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QString TYPE{QLatin1String("CHANGE_RESOLUTION")};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
std::optional<ChangeResolution> ChangeResolution::fromJson(const QJsonDocument& data)
{
    if (data.isObject())
    {
        const auto obj_v       = data.object();
        const auto type_v      = obj_v.value(QLatin1String("type"));
        const auto width_v     = obj_v.value(QLatin1String("width"));
        const auto height_v    = obj_v.value(QLatin1String("height"));
        const auto immediate_v = obj_v.value(QLatin1String("immediate"));

        if (type_v.isString() && type_v.toString() == TYPE)
        {
            if (width_v.isDouble() && width_v.toInt(-1) > 0 && height_v.isDouble() && height_v.toInt(-1) > 0
                && immediate_v.isBool())
            {
                return ChangeResolution{static_cast<uint>(width_v.toInt()), static_cast<uint>(height_v.toInt()),
                                        immediate_v.toBool()};
            }
        }
    }
    return std::nullopt;
}
}  // namespace msgs::in