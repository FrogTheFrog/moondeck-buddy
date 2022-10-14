#pragma once

// system/Qt includes
#include <QJsonDocument>

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::out
{
struct ControlTypeTaken
{
    static QJsonDocument toJson(const ControlTypeTaken& data);
};
}  // namespace msgs::out
