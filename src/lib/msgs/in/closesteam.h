#pragma once

// system/Qt includes
#include <QJsonDocument>

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
struct CloseSteam
{
    static std::optional<CloseSteam> fromJson(const QJsonDocument& data);

    std::optional<uint> m_grace_period;
};
}  // namespace msgs::in
