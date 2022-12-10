#pragma once

// system/Qt includes
#include <QJsonDocument>

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
struct PairStatus
{
    static std::optional<PairStatus> fromJson(const QJsonDocument& data);

    QString m_id;
};
}  // namespace msgs::in
