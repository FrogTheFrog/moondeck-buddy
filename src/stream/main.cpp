// system/Qt includes
#include <QCoreApplication>

// local includes
#include "shared/appmetadata.h"
#include "shared/loggingcategories.h"
#include "utils/heartbeat.h"
#include "utils/logsettings.h"
#include "utils/singleinstanceguard.h"
#include "utils/unixsignalhandler.h"

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-avoid-c-arrays)
int main(int argc, char* argv[])
{
    const shared::AppMetadata app_meta{shared::AppMetadata::App::Stream};

    utils::SingleInstanceGuard guard{app_meta.getAppName()};
    if (!guard.tryToRun())
    {
        return EXIT_SUCCESS;
    }

    QCoreApplication app{argc, argv};
    QCoreApplication::setApplicationName(app_meta.getAppName());

    utils::installSignalHandler();
    utils::LogSettings::getInstance().init(app_meta.getLogPath());
    qCInfo(lc::streamMain) << "startup. Version:" << EXEC_VERSION;

    utils::Heartbeat heartbeat{app_meta.getAppName()};
    QObject::connect(&heartbeat, &utils::Heartbeat::signalShouldTerminate, &app, &QCoreApplication::quit);
    heartbeat.startBeating();

    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() { qCInfo(lc::streamMain) << "shutdown."; });
    qCInfo(lc::streamMain) << "startup finished.";
    return QCoreApplication::exec();
}
