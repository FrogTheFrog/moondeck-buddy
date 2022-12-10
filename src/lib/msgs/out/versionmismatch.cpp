// header file include
#include "versionmismatch.h"

// system/Qt includes
#include <QJsonObject>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QString TYPE{QLatin1String("VERSION_MISMATCH")};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::out
{
QJsonDocument VersionMismatch::toJson(const VersionMismatch& data)
{
    QJsonObject obj;

    obj["type"]    = TYPE;
    obj["version"] = data.m_version;

    return QJsonDocument{obj};
}
}  // namespace msgs::out