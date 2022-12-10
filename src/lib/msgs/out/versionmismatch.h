#pragma once

// system/Qt includes
#include <QJsonDocument>

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::out
{
struct VersionMismatch
{
    static QJsonDocument toJson(const VersionMismatch& data);

    int m_version;
};
}  // namespace msgs::out
