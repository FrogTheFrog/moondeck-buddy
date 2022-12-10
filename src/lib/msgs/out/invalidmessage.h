#pragma once

// system/Qt includes
#include <QJsonDocument>

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::out
{
struct InvalidMessage
{
    static QJsonDocument toJson(const InvalidMessage& data);
};
}  // namespace msgs::out
