// header file include
#include "pair.h"

// system/Qt includes
#include <QJsonObject>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QString TYPE{QLatin1String("PAIR")};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
std::optional<Pair> Pair::fromJson(const QJsonDocument& data)
{
    if (data.isObject())
    {
        const auto obj_v       = data.object();
        const auto type_v      = obj_v.value(QLatin1String("type"));
        const auto id_v        = obj_v.value(QLatin1String("id"));
        const auto hashed_id_v = obj_v.value(QLatin1String("hashed_id"));

        if (type_v.isString() && type_v.toString() == TYPE)
        {
            if (id_v.isString() && hashed_id_v.isString())
            {
                return Pair{id_v.toString(), hashed_id_v.toString()};
            }
        }
    }
    return std::nullopt;
}
}  // namespace msgs::in