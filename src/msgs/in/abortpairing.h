#pragma once

// system/Qt includes
#include <QJsonDocument>

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
struct AbortPairing
{
    static std::optional<AbortPairing> fromJson(const QJsonDocument& data);

    QString m_id;
};
}  // namespace msgs::in
