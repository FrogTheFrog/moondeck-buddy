// system/Qt includes
#include <QApplication>
#include <QDir>
#include <QMainWindow>

// local includes
#include "os/pccontrol.h"
#include "os/sunshineapps.h"
#include "routing.h"
#include "server/clientids.h"
#include "server/httpserver.h"
#include "server/pairingmanager.h"
#include "shared/appmetadata.h"
#include "shared/loggingcategories.h"
#include "utils/appsettings.h"
#include "utils/logsettings.h"
#include "utils/pairinginput.h"
#include "utils/singleinstanceguard.h"
#include "utils/systemtray.h"
#include "utils/unixsignalhandler.h"

// TODO: remove hack
#if defined(Q_OS_WIN)
    #include "os/win/streamstatehandler.h"
#endif

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-avoid-c-arrays)
int main(int argc, char* argv[])
{
    constexpr int             api_version{3};
    const shared::AppMetadata app_meta{shared::AppMetadata::App::Buddy};

    utils::SingleInstanceGuard guard{app_meta.getAppName()};
    if (!guard.tryToRun())
    {
        return EXIT_SUCCESS;
    }

    QApplication app{argc, argv};
    QCoreApplication::setApplicationName(app_meta.getAppName());

    utils::installSignalHandler();
    utils::LogSettings::getInstance().init(app_meta.getLogPath());
    qCInfo(lc::buddyMain) << "startup.";

    const utils::AppSettings app_settings{app_meta.getSettingsPath()};
    utils::LogSettings::getInstance().setLoggingRules(app_settings.getLoggingRules());

    server::ClientIds      client_ids{QDir::cleanPath(app_meta.getSettingsDir() + "/clients.json")};
    server::HttpServer     new_server{api_version, client_ids};
    server::PairingManager pairing_manager{client_ids};

    os::PcControl    pc_control{app_meta, app_settings.getHandledDisplays()};
    os::SunshineApps sunshine_apps{app_settings.getSunshineAppsFilepath()};

    const QIcon               icon{":/icons/app.ico"};
    const utils::SystemTray   tray{icon, app_meta.getAppName(), pc_control};
    const utils::PairingInput pairing_input;

    // Tray + app
    QObject::connect(&tray, &utils::SystemTray::signalQuitApp, &app, &QApplication::quit);

    // Tray + pc control
    QObject::connect(&pc_control, &os::PcControl::signalShowTrayMessage, &tray,
                     &utils::SystemTray::slotShowTrayMessage);

    // Pairing manager + pairing input
    QObject::connect(&pairing_manager, &server::PairingManager::signalRequestUserInputForPairing, &pairing_input,
                     &utils::PairingInput::slotRequestUserInputForPairing);
    QObject::connect(&pairing_manager, &server::PairingManager::signalAbortPairing, &pairing_input,
                     &utils::PairingInput::slotAbortPairing);
    QObject::connect(&pairing_input, &utils::PairingInput::signalFinishPairing, &pairing_manager,
                     &server::PairingManager::slotFinishPairing);
    QObject::connect(&pairing_input, &utils::PairingInput::signalPairingRejected, &pairing_manager,
                     &server::PairingManager::slotPairingRejected);

    // HERE WE GO!!! (a.k.a. starting point)
    setupRoutes(new_server, pairing_manager, pc_control, sunshine_apps, app_settings.getPreferHibernation());

    client_ids.load();
    if (!new_server.startServer(app_settings.getPort(), ":/ssl/moondeck_cert.pem", ":/ssl/moondeck_key.pem"))
    {
        qFatal("Failed to start server!");
    }

// TODO: remove once Nvidia kills GameStream
#if defined(Q_OS_WIN)
    getMouseAccelResetHack() = app_settings.m_nvidia_reset_mouse_acceleration_after_stream_end_hack;
#endif

    QGuiApplication::setQuitOnLastWindowClosed(false);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() { qCInfo(lc::buddyMain) << "shutdown."; });
    qCInfo(lc::buddyMain) << "startup finished.";
    return QCoreApplication::exec();
}
