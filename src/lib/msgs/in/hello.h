#pragma once

// system/Qt includes
#include <QJsonDocument>

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
struct Hello
{
    static std::optional<Hello> fromJson(const QJsonDocument& data);

    int m_version;
};
}  // namespace msgs::in
