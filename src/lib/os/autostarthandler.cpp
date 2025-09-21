// header file include
#include "os/autostarthandler.h"

// system/Qt includes
#include <QtGlobal>

// os-specific includes
#if defined(Q_OS_WIN)
    #include "os/win/nativeautostarthandler.h"
#elif defined(Q_OS_LINUX)
    #include "os/linux/nativeautostarthandler.h"
#else
    #error OS is not supported!
#endif

namespace os
{
AutoStartHandler::AutoStartHandler(const shared::AppMetadata& app_meta)
    : m_impl{std::make_unique<NativeAutoStartHandler>(app_meta)}
{
}

AutoStartHandler::~AutoStartHandler() = default;

void AutoStartHandler::setAutoStart(bool enable)
{
    m_impl->setAutoStart(enable);
}

bool AutoStartHandler::isAutoStartEnabled() const
{
    return m_impl->isAutoStartEnabled();
}

bool AutoStartHandler::isServiceSupported() const
{
    return m_impl->isServiceSupported();
}

bool AutoStartHandler::restartIntoService()
{
    return m_impl->restartIntoService();
}
}  // namespace os
