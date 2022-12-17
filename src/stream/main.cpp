// system/Qt includes
#include <QApplication>

// local includes
#include "shared/constants.h"
#include "utils/heartbeat.h"
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

    QApplication     app{argc, argv};
    utils::Heartbeat heartbeat{shared::APP_NAME_STREAM};

    QObject::connect(&heartbeat, &utils::Heartbeat::signalShouldTerminate, &app, &QCoreApplication::quit);

    heartbeat.startBeating();
    return QCoreApplication::exec();
}
