#pragma once

// system/Qt includes
#include <QJsonDocument>

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
struct Login
{
    enum class ControlType
    {
        Steam,
        Pc
    };

    static std::optional<Login> fromJson(const QJsonDocument& data);

    QString     m_id;
    ControlType m_constrol_type;
};
}  // namespace msgs::in
