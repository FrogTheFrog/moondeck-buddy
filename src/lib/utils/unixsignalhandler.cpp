// header file include
#include "utils/unixsignalhandler.h"

// system/Qt includes
#include <csignal>

// local includes
#include "shared/loggingcategories.h"

namespace
{
volatile sig_atomic_t SIGNALED_CODE{0};

void log_signal()
{
    qCInfo(lc::utils).nospace().noquote() << "interrupted by signal " << SIGNALED_CODE << ([]()
    {
        switch (SIGNALED_CODE)
        {
            case SIGINT:
                return " (SIGINT)";
            case SIGTERM:
                return " (SIGTERM)";
#if defined(Q_OS_LINUX)
            case SIGHUP:
                return " (SIGHUP)";
            case SIGQUIT:
                return " (SIGQUIT)";
#endif
            default:
                return "";
        }
    }());
}

void handler(const int code)
{
    SIGNALED_CODE = code;
    std::quick_exit(0);
}
}  // namespace

namespace utils
{
void installSignalHandler()
{
    std::at_quick_exit(log_signal);
    std::signal(SIGINT, handler);
    std::signal(SIGTERM, handler);
#if defined(Q_OS_LINUX)
    std::signal(SIGHUP, handler);
    std::signal(SIGQUIT, handler);
#endif
}
}  // namespace utils
