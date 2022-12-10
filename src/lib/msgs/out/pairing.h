#pragma once

// system/Qt includes
#include <QJsonDocument>

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::out
{
struct Pairing
{
    static QJsonDocument toJson(const Pairing& data);
};
}  // namespace msgs::out
