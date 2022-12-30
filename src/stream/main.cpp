// system/Qt includes
#include <QCoreApplication>

// local includes
#include "shared/constants.h"
#include "utils/heartbeat.h"
#include "utils/logsettings.h"
#include "utils/singleinstanceguard.h"

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-avoid-c-arrays)
int main(int argc, char* argv[])
{
    utils::SingleInstanceGuard guard{shared::APP_NAME_STREAM};
    if (!guard.tryToRun())
    {
        return EXIT_SUCCESS;
    }

    QCoreApplication app{argc, argv};
    utils::LogSettings::getInstance().init("stream.log");

    utils::Heartbeat heartbeat{shared::APP_NAME_STREAM};
    QObject::connect(&heartbeat, &utils::Heartbeat::signalShouldTerminate, &app, &QCoreApplication::quit);
    heartbeat.startBeating();

    return QCoreApplication::exec();
}
