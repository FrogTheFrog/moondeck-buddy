#pragma once

// system/Qt includes
#include <QtDBus/QDBusUnixFileDescriptor>

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

private:
    QDBusUnixFileDescriptor m_file_descriptor;
};
}  // namespace os
