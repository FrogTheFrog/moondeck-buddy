// header file include
#include "messageaccepted.h"

// system/Qt includes
#include <QJsonObject>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QString TYPE{QLatin1String("MESSAGE_ACCEPTED")};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::out
{
QJsonDocument MessageAccepted::toJson(const MessageAccepted& /*data*/)
{
    QJsonObject obj;

    obj["type"] = TYPE;

    return QJsonDocument{obj};
}
}  // namespace msgs::out