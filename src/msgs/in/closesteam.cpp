// header file include
#include "closesteam.h"

// system/Qt includes
#include <QJsonObject>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QString TYPE{QLatin1String("CLOSE_STEAM")};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
std::optional<CloseSteam> CloseSteam::fromJson(const QJsonDocument& data)
{
    if (data.isObject())
    {
        const auto obj_v            = data.object();
        const auto type_v           = obj_v.value(QLatin1String("type"));
        const auto m_grace_period_v = obj_v.value(QLatin1String("grace_period"));

        if (type_v.isString() && type_v.toString() == TYPE)
        {
            if (m_grace_period_v.isDouble() || m_grace_period_v.isNull())
            {
                const int                 max_time_limit{30};
                const std::optional<uint> time = m_grace_period_v.isDouble() ? std::make_optional(static_cast<uint>(
                                                     std::max(std::min(m_grace_period_v.toInt(), 0), max_time_limit)))
                                                                             : std::nullopt;
                return CloseSteam{time};
            }
        }
    }
    return std::nullopt;
}
}  // namespace msgs::in