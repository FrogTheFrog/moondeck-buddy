// header file include
#include "os/processhandler.h"

// system/Qt includes

// os-specific includes
#if defined(Q_OS_WIN)
    #include "os/win/nativeprocesshandler.h"
#elif defined(Q_OS_LINUX)
    #include "os/linux/nativeprocesshandler.h"
#else
    #error OS is not supported!
#endif

// local includes
#include "common/loggingcategories.h"

namespace os
{
ProcessHandler::ProcessHandler()
    : m_native_handler{std::make_unique<NativeProcessHandler>()}
{
}

ProcessHandler::~ProcessHandler() = default;

std::vector<uint> ProcessHandler::getPids() const
{
    return m_native_handler->getPids();
}

QString ProcessHandler::getExecPath(uint pid) const
{
    return m_native_handler->getExecPath(pid);
}

QDateTime ProcessHandler::getStartTime(uint pid) const
{
    return m_native_handler->getStartTime(pid);
}

void ProcessHandler::close(uint pid) const
{
    return m_native_handler->close(pid);
}

void ProcessHandler::terminate(uint pid) const
{
    return m_native_handler->terminate(pid);
}
}  // namespace os
