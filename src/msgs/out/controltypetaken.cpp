// header file include
#include "controltypetaken.h"

// system/Qt includes
#include <QJsonObject>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QString TYPE{QLatin1String("CONTROL_TYPE_TAKEN")};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::out
{
QJsonDocument ControlTypeTaken::toJson(const ControlTypeTaken& /*data*/)
{
    QJsonObject obj;

    obj["type"] = TYPE;

    return QJsonDocument{obj};
}
}  // namespace msgs::out