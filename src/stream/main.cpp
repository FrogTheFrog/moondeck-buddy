// system/Qt includes
#include <QCoreApplication>

// local includes
#include "os/sleepinhibitor.h"
#include "shared/appmetadata.h"
#include "shared/loggingcategories.h"
#include "utils/heartbeat.h"
#include "utils/logsettings.h"
#include "utils/singleinstanceguard.h"
#include "utils/unixsignalhandler.h"
#include "utils/envsharedmemory.h"

// NOLINTNEXTLINE(*-avoid-c-arrays)
int main(int argc, char* argv[])
{
    const shared::AppMetadata  app_meta{shared::AppMetadata::App::Stream};
    utils::SingleInstanceGuard guard{app_meta.getAppName()};

    QCoreApplication app{argc, argv};
    QCoreApplication::setApplicationName(app_meta.getAppName());
    QCoreApplication::setApplicationVersion(EXEC_VERSION);

    if (!guard.tryToRun())
    {
        qCWarning(lc::streamMain) << "another instance of" << app_meta.getAppName() << "is already running!";
        return EXIT_FAILURE;
    }

    utils::installSignalHandler();
    utils::LogSettings::getInstance().init(app_meta.getLogPath());
    qCInfo(lc::streamMain) << "startup. Version:" << EXEC_VERSION;

    // Capture and store environment variables for Buddy to use when launching games
    utils::EnvSharedMemory envMemory;
    if (!envMemory.captureAndStoreEnvironment({"APOLLO", "SUNSHINE"}))
    {
        qCWarning(lc::streamMain) << "Failed to capture environment variables, but continuing...";
    }

    const os::SleepInhibitor sleep_inhibitor{app_meta.getAppName()};
    utils::Heartbeat         heartbeat{app_meta.getAppName()};
    QObject::connect(&heartbeat, &utils::Heartbeat::signalShouldTerminate, &app, &QCoreApplication::quit);
    
    // Clear environment variables when the application exits
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&envMemory]() { 
        envMemory.clearEnvironment();
        qCInfo(lc::streamMain) << "shutdown."; 
    });
    
    heartbeat.startBeating();

    qCInfo(lc::streamMain) << "startup finished.";

    return QCoreApplication::exec();
}
