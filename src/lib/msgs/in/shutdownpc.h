#pragma once

// system/Qt includes
#include <QJsonDocument>

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
struct ShutdownPc
{
    static std::optional<ShutdownPc> fromJson(const QJsonDocument& data);

    uint m_grace_period;
};
}  // namespace msgs::in
