// system/Qt includes
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QMainWindow>

// local includes
#include "os/autostarthandler.h"
#include "os/pccontrol.h"
#include "os/sunshineapps.h"
#include "os/systemtray.h"
#include "routing.h"
#include "server/clientids.h"
#include "server/httpserver.h"
#include "server/pairingmanager.h"
#include "shared/appmetadata.h"
#include "shared/loggingcategories.h"
#include "utils/appsettings.h"
#include "utils/heartbeat.h"
#include "utils/logsettings.h"
#include "utils/pairinginput.h"
#include "utils/singleinstanceguard.h"
#include "utils/unixsignalhandler.h"

namespace
{
std::optional<int> parseArguments(QCommandLineParser& parser, const shared::AppMetadata& app_meta)
{
    const QCommandLineOption help_option{{"h", "help"}, "Displays help on commandline options."};
    const QCommandLineOption version_option{{"v", "version"}, "Displays version information."};
    const QCommandLineOption enable_autostart_option{"enable-autostart", "Enable the autostart for Buddy."};
    const QCommandLineOption disable_autostart_option{"disable-autostart", "Disable the autostart for Buddy."};
    const QCommandLineOption close_all_option{"close-all", "Close running Buddy and Stream instances."};

    parser.addOption(help_option);
    parser.addOption(version_option);
    parser.addOption(enable_autostart_option);
    parser.addOption(disable_autostart_option);
    parser.addOption(close_all_option);

    if (!parser.parse(QCoreApplication::arguments()))
    {
        qInfo() << "Failed to parse arguments:" << parser.unknownOptionNames();
        return EXIT_FAILURE;
    }

    if (const auto unknown_args = parser.positionalArguments(); !unknown_args.empty())
    {
        qInfo() << "Failed to parse unknown arguments:" << unknown_args;
        return EXIT_FAILURE;
    }

    if (parser.isSet(help_option))
    {
        parser.showHelp();
        Q_UNREACHABLE_RETURN(EXIT_SUCCESS);
    }

    if (parser.isSet(version_option))
    {
        parser.showVersion();
        Q_UNREACHABLE_RETURN(EXIT_SUCCESS);
    }

    std::optional<int> return_code;
    if (parser.isSet(enable_autostart_option) || parser.isSet(disable_autostart_option))
    {
        const bool           enable{parser.isSet(enable_autostart_option)};
        os::AutoStartHandler auto_start_handler{app_meta};
        auto_start_handler.setAutoStart(enable);

        if (enable != auto_start_handler.isAutoStartEnabled())
        {
            qInfo() << "Failed to enable autostart.";
            return EXIT_FAILURE;
        }

        return_code = EXIT_SUCCESS;
    }

    if (parser.isSet(close_all_option))
    {
        utils::Heartbeat buddy_heartbeat{app_meta.getAppName(shared::AppMetadata::App::Buddy)};
        utils::Heartbeat stream_heartbeat{app_meta.getAppName(shared::AppMetadata::App::Stream)};

        buddy_heartbeat.startListening();
        stream_heartbeat.startListening();

        buddy_heartbeat.terminate();
        stream_heartbeat.terminate();

        {
            constexpr int terminate_timeout_ms{5000};
            const auto    listener{[&buddy_heartbeat, &stream_heartbeat](const bool timeout)
                                {
                                    if (!timeout && (buddy_heartbeat.isAlive() || stream_heartbeat.isAlive()))
                                    {
                                        return;
                                    }

                                    QCoreApplication::quit();
                                }};

            const QObject anchor;
            QTimer::singleShot(terminate_timeout_ms, &anchor, [&listener]() { listener(true); });
            QTimer::singleShot(0, &anchor, [&listener]() { listener(false); });
            QObject::connect(&buddy_heartbeat, &utils::Heartbeat::signalStateChanged, &anchor,
                             [&listener]() { listener(false); });
            QObject::connect(&stream_heartbeat, &utils::Heartbeat::signalStateChanged, &anchor,
                             [&listener]() { listener(false); });

            return_code = QCoreApplication::exec();
        }

        if (return_code == EXIT_SUCCESS)
        {
            if (buddy_heartbeat.isAlive() || stream_heartbeat.isAlive())
            {
                qInfo() << "Failed to terminate active Buddy and/or Stream instance.";
                return EXIT_FAILURE;
            }
        }
    }

    return return_code;
}
}  // namespace

// NOLINTNEXTLINE(*-avoid-c-arrays)
int main(int argc, char* argv[])
{
    constexpr int              api_version{7};
    const shared::AppMetadata  app_meta{shared::AppMetadata::App::Buddy};
    utils::SingleInstanceGuard guard{app_meta.getAppName()};

    auto app{[&]() -> std::unique_ptr<QCoreApplication>
             {
                 if (app_meta.isGuiEnabled())
                 {
                     auto gui_app = std::make_unique<QApplication>(argc, argv);
                     gui_app->setQuitOnLastWindowClosed(false);
                     return gui_app;
                 }

                 return std::make_unique<QCoreApplication>(argc, argv);
             }()};
    QCoreApplication::setApplicationName(app_meta.getAppName());
    QCoreApplication::setApplicationVersion(EXEC_VERSION);

    QCommandLineParser parser;
    if (const auto result = parseArguments(parser, app_meta); result)
    {
        return *result;
    }

    if (!guard.tryToRun())
    {
        qCWarning(lc::buddyMain) << "another instance of" << app_meta.getAppName() << "is already running!";
        return EXIT_FAILURE;
    }

    utils::installSignalHandler();
    utils::LogSettings::getInstance().init(app_meta.getLogPath());
    qCInfo(lc::buddyMain) << "startup. Version:" << EXEC_VERSION;

    utils::Heartbeat heartbeat{app_meta.getAppName()};
    QObject::connect(&heartbeat, &utils::Heartbeat::signalShouldTerminate, app.get(), &QCoreApplication::quit);
    heartbeat.startBeating();

    const utils::AppSettings app_settings{app_meta};
    utils::LogSettings::getInstance().setLoggingRules(app_settings.getLoggingRules());

    // Capture and store environment variable regex for Stream to save when it's started
    utils::ShmSerializer env_regex_serializer{app_meta.getSharedEnvRegexKey()};
    if (!env_regex_serializer.write(app_settings.getEnvCaptureRegex()))
    {
        qCWarning(lc::buddyMain) << "Failed to write ENV regex to the shared memory, still continuing...";
    }

    server::ClientIds      client_ids{QDir::cleanPath(app_meta.getSettingsDir() + "/clients.json")};
    server::HttpServer     new_server{api_version, client_ids};
    server::PairingManager pairing_manager{client_ids, app_meta.isGuiEnabled()};

    os::PcControl    pc_control{app_settings};
    os::SunshineApps sunshine_apps{app_settings.getSunshineAppsFilepath()};

    std::unique_ptr<QIcon>               icon;
    std::unique_ptr<os::SystemTray>      tray;
    std::unique_ptr<utils::PairingInput> pairing_input;

    if (app_meta.isGuiEnabled())
    {
        icon          = std::make_unique<QIcon>(QIcon::fromTheme("moondeckbuddy", QIcon{":/icons/moondeckbuddy.ico"}));
        tray          = std::make_unique<os::SystemTray>(*icon, app_meta.getAppName(), pc_control);
        pairing_input = std::make_unique<utils::PairingInput>();

        // Tray + app
        QObject::connect(tray.get(), &os::SystemTray::signalQuitApp, app.get(), &QCoreApplication::quit);

        // Tray + pc control
        QObject::connect(&pc_control, &os::PcControl::signalShowTrayMessage, tray.get(),
                         &os::SystemTray::slotShowTrayMessage);

        // Pairing manager + pairing input
        QObject::connect(&pairing_manager, &server::PairingManager::signalRequestUserInputForPairing,
                         pairing_input.get(), &utils::PairingInput::slotRequestUserInputForPairing);
        QObject::connect(&pairing_manager, &server::PairingManager::signalAbortPairing, pairing_input.get(),
                         &utils::PairingInput::slotAbortPairing);
        QObject::connect(pairing_input.get(), &utils::PairingInput::signalFinishPairing, &pairing_manager,
                         &server::PairingManager::slotFinishPairing);
        QObject::connect(pairing_input.get(), &utils::PairingInput::signalPairingRejected, &pairing_manager,
                         &server::PairingManager::slotPairingRejected);
    }
    else
    {
        qCInfo(lc::buddyMain) << "NO_GUI mode enabled.";
    }

    // HERE WE GO!!! (a.k.a. starting point)
    setupRoutes(new_server, pairing_manager, pc_control, sunshine_apps, app_settings.getMacAddressOverride());

    client_ids.load();
    if (!new_server.startServer(app_settings.getPort(), ":/ssl/moondeck_cert.pem", ":/ssl/moondeck_key.pem",
                                app_settings.getSslProtocol()))
    {
        qFatal("Failed to start server!");
    }

    QObject::connect(app.get(), &QCoreApplication::aboutToQuit, []() { qCInfo(lc::buddyMain) << "shutdown."; });
    qCInfo(lc::buddyMain) << "startup finished.";
    return QCoreApplication::exec();
}
