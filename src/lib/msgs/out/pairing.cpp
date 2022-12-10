// header file include
#include "pairing.h"

// system/Qt includes
#include <QJsonObject>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QString TYPE{QLatin1String("PAIRING")};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::out
{
QJsonDocument Pairing::toJson(const Pairing& /*data*/)
{
    QJsonObject obj;

    obj["type"] = TYPE;

    return QJsonDocument{obj};
}
}  // namespace msgs::out