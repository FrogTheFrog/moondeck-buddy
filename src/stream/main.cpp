// system/Qt includes
#include <QCoreApplication>
#include <QProcessEnvironment>
#include <QRegularExpression>

// local includes
#include "os/sleepinhibitor.h"
#include "shared/appmetadata.h"
#include "shared/loggingcategories.h"
#include "utils/heartbeat.h"
#include "utils/logsettings.h"
#include "utils/shmserialization.h"
#include "utils/singleinstanceguard.h"
#include "utils/unixsignalhandler.h"

QMap<QString, QString> getMatchingEnv(const QRegularExpression& regex)
{
    QMap<QString, QString> captured_env;
    for (const auto env{QProcessEnvironment::systemEnvironment()}; const QString& key : env.keys())
    {
        if (const auto match = regex.match(key); match.hasMatch())
        {
            const auto value{env.value(key)};
            qCDebug(lc::streamMain) << "Captured environment variable:" << key << "=" << value;
            captured_env[key] = value;
        }
    }

    return captured_env;
}

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

    utils::LogSettings::getInstance().init(app_meta.getLogPath());
    utils::installSignalHandler();
    qCInfo(lc::streamMain) << "startup. Version:" << EXEC_VERSION;

    // Capture and store environment variables for Buddy to use when launching games
    utils::ShmSerializer env_map_serializer{app_meta.getSharedEnvMapKey()};
    {
        utils::ShmDeserializer env_regex_deserializer{app_meta.getSharedEnvRegexKey()};
        if (const auto regex = env_regex_deserializer.read<QRegularExpression>())
        {
            qCInfo(lc::streamMain) << "Got the following ENV regex from Buddy:" << *regex;
            if (!env_map_serializer.write(getMatchingEnv(*regex)))
            {
                qCWarning(lc::streamMain) << "Failed to capture environment variables, still continuing...";
            }
        }
        else
        {
            qCWarning(lc::streamMain) << "Failed to read ENV regex from shared memory!";
        }
    }

    const os::SleepInhibitor sleep_inhibitor{app_meta.getAppName()};
    utils::Heartbeat         heartbeat{app_meta.getAppName()};
    QObject::connect(&heartbeat, &utils::Heartbeat::signalShouldTerminate, &app, &QCoreApplication::quit);
    heartbeat.startBeating();

    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() { qCInfo(lc::streamMain) << "shutdown."; });
    qCInfo(lc::streamMain) << "startup finished.";

    return QCoreApplication::exec();
}
