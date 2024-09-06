// header file include
#include "utils/unixsignalhandler.h"

// system/Qt includes
#include <QCoreApplication>
#include <csignal>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
void handler(int code)
{
    std::signal(code, SIG_DFL);
    QCoreApplication::quit();
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace utils
{
void installSignalHandler()
{
    Q_ASSERT(QCoreApplication::instance() != nullptr);

    std::signal(SIGINT, handler);
    std::signal(SIGTERM, handler);
}
}  // namespace utils
