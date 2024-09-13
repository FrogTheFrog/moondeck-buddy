// header file include
#include "os/sleepinhibitor.h"

// system/Qt includes
#include <QtGlobal>

// os-specific includes
#if defined(Q_OS_WIN)
    #include "os/win/nativesleepinhibitor.h"
#elif defined(Q_OS_LINUX)
    #include "os/linux/nativesleepinhibitor.h"
#else
    #error OS is not supported!
#endif

namespace os
{
SleepInhibitor::SleepInhibitor(const QString& app_name)
    : m_impl{std::make_unique<NativeSleepInhibitor>(app_name)}
{
}

// For forward declarations
SleepInhibitor::~SleepInhibitor() = default;
}  // namespace os
