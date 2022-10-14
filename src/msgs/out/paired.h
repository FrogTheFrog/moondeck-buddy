#pragma once

// system/Qt includes
#include <QJsonDocument>

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::out
{
struct Paired
{
    static QJsonDocument toJson(const Paired& data);
};
}  // namespace msgs::out
