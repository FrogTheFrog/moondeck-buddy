#pragma once

// system/Qt includes
#include <QJsonDocument>

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::out
{
struct MessageAccepted
{
    static QJsonDocument toJson(const MessageAccepted& data);
};
}  // namespace msgs::out
