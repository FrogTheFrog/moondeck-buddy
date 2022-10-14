// header file include
#include "invalidmessage.h"

// system/Qt includes
#include <QJsonObject>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QString TYPE{QLatin1String("INVALID_MESSAGE")};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::out
{
QJsonDocument InvalidMessage::toJson(const InvalidMessage& /*data*/)
{
    QJsonObject obj;

    obj["type"] = TYPE;

    return QJsonDocument{obj};
}
}  // namespace msgs::out