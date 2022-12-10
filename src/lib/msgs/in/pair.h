#pragma once

// system/Qt includes
#include <QJsonDocument>

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
struct Pair
{
    static std::optional<Pair> fromJson(const QJsonDocument& data);

    QString m_id;
    QString m_hashed_id;
};
}  // namespace msgs::in
