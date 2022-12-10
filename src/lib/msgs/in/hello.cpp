// header file include
#include "hello.h"

// system/Qt includes
#include <QJsonObject>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QString TYPE{QLatin1String("HELLO")};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
std::optional<Hello> Hello::fromJson(const QJsonDocument& data)
{
    if (data.isObject())
    {
        const auto obj_v     = data.object();
        const auto type_v    = obj_v.value(QLatin1String("type"));
        const auto version_v = obj_v.value(QLatin1String("version"));

        if (type_v.isString() && type_v.toString() == TYPE && version_v.isDouble())
        {
            return Hello{version_v.toInt()};
        }
    }
    return std::nullopt;
}
}  // namespace msgs::in