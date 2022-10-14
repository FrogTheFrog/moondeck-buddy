// header file include
#include "pairstatus.h"

// system/Qt includes
#include <QJsonObject>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QString TYPE{QLatin1String("PAIR_STATUS")};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
std::optional<PairStatus> PairStatus::fromJson(const QJsonDocument& data)
{
    if (data.isObject())
    {
        const auto obj_v  = data.object();
        const auto type_v = obj_v.value(QLatin1String("type"));
        const auto id_v   = obj_v.value(QLatin1String("id"));

        if (type_v.isString() && type_v.toString() == TYPE)
        {
            if (id_v.isString())
            {
                return PairStatus{id_v.toString()};
            }
        }
    }
    return std::nullopt;
}
}  // namespace msgs::in