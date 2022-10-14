#pragma once

// system/Qt includes
#include <QJsonDocument>

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
struct LaunchApp
{
    static std::optional<LaunchApp> fromJson(const QJsonDocument& data);

    uint m_app_id;
};
}  // namespace msgs::in
