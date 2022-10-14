#pragma once

// system/Qt includes
#include <QJsonDocument>

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::out
{
struct NotPaired
{
    static QJsonDocument toJson(const NotPaired& data);
};
}  // namespace msgs::out
