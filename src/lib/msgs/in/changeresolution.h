#pragma once

// system/Qt includes
#include <QJsonDocument>

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
struct ChangeResolution
{
    static std::optional<ChangeResolution> fromJson(const QJsonDocument& data);

    uint m_width;
    uint m_height;
    bool m_immediate;
};
}  // namespace msgs::in
