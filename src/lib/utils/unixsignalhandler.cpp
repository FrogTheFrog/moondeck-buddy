// header file include
#include "unixsignalhandler.h"

// system/Qt includes
#include <QCoreApplication>
#include <csignal>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
void handler(int code)
{
    std::signal(code, SIG_DFL);
    QCoreApplication::instance()->quit();
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
