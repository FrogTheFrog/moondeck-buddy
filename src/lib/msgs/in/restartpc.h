#pragma once

// system/Qt includes
#include <QJsonDocument>

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
struct RestartPc
{
    static std::optional<RestartPc> fromJson(const QJsonDocument& data);

    uint m_grace_period;
};
}  // namespace msgs::in
