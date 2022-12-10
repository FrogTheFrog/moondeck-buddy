// header file include
#include "restartpc.h"

// system/Qt includes
#include <QJsonObject>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QString TYPE{QLatin1String("RESTART_PC")};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
std::optional<RestartPc> RestartPc::fromJson(const QJsonDocument& data)
{
    if (data.isObject())
    {
        const auto obj_v            = data.object();
        const auto type_v           = obj_v.value(QLatin1String("type"));
        const auto m_grace_period_v = obj_v.value(QLatin1String("grace_period"));

        if (type_v.isString() && type_v.toString() == TYPE)
        {
            if (m_grace_period_v.isDouble())
            {
                const int  max_time_limit{30};
                const uint time = static_cast<uint>(std::max(std::min(m_grace_period_v.toInt(), 0), max_time_limit));
                return RestartPc{time};
            }
        }
    }
    return std::nullopt;
}
}  // namespace msgs::in