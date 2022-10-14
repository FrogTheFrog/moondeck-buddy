// header file include
#include "pcstate.h"

// system/Qt includes
#include <QJsonObject>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QString TYPE{QLatin1String("PC_STATE")};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
std::optional<PcState> PcState::fromJson(const QJsonDocument& data)
{
    if (data.isObject())
    {
        const auto obj_v  = data.object();
        const auto type_v = obj_v.value(QLatin1String("type"));

        if (type_v.isString() && type_v.toString() == TYPE)
        {
            return PcState{};
        }
    }
    return std::nullopt;
}
}  // namespace msgs::in