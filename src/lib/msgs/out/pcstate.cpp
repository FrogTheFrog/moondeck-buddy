// header file include
#include "pcstate.h"

// system/Qt includes
#include <QJsonObject>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QString TYPE{QLatin1String("PC_STATE")};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::out
{
QJsonDocument PcState::toJson(const PcState& data)
{
    QJsonObject obj;

    obj["type"]     = TYPE;
    obj["pc_state"] = QVariant::fromValue(data.m_pc_state).toString();

    return QJsonDocument{obj};
}
}  // namespace msgs::out