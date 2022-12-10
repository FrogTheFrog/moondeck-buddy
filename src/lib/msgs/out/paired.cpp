// header file include
#include "paired.h"

// system/Qt includes
#include <QJsonObject>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QString TYPE{QLatin1String("PAIRED")};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::out
{
QJsonDocument Paired::toJson(const Paired& /*data*/)
{
    QJsonObject obj;

    obj["type"] = TYPE;

    return QJsonDocument{obj};
}
}  // namespace msgs::out