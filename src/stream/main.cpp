// system/Qt includes
#include <QCoreApplication>

// local includes
#include "os/sleepinhibitor.h"
#include "os/steam/steamcontentlogtracker.h"
#include "shared/appmetadata.h"
#include "shared/loggingcategories.h"
#include "utils/heartbeat.h"
#include "utils/logsettings.h"
#include "utils/singleinstanceguard.h"
#include "utils/unixsignalhandler.h"

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

    const os::SleepInhibitor sleep_inhibitor{app_meta.getAppName()};
    utils::Heartbeat         heartbeat{app_meta.getAppName()};
    QObject::connect(&heartbeat, &utils::Heartbeat::signalShouldTerminate, &app, &QCoreApplication::quit);
    heartbeat.startBeating();

    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() { qCInfo(lc::streamMain) << "shutdown."; });
    qCInfo(lc::streamMain) << "startup finished.";

    os::SteamContentLogTracker tracker{"/home/frog/Desktop/"};

    return QCoreApplication::exec();
}
