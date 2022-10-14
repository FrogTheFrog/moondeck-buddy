#pragma once

// system/Qt includes
#include <QJsonDocument>

//---------------------------------------------------------------------------------------------------------------------

namespace msgs::in
{
struct PcState
{
    static std::optional<PcState> fromJson(const QJsonDocument& data);
};
}  // namespace msgs::in
