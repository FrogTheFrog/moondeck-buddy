#pragma once

// system/Qt includes
#include <QString>

// local includes
#include "os/shared/nativesleepinhibitorinterface.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class NativeSleepInhibitor : public NativeSleepInhibitorInterface
{
public:
    explicit NativeSleepInhibitor(const QString& app_name);
    ~NativeSleepInhibitor() override = default;
};
}  // namespace os
