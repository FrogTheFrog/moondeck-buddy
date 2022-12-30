// system/Qt includes
#include <QApplication>
#include <QMainWindow>

// local includes
#include "os/pccontrol.h"
#include "routing.h"
#include "server/clientids.h"
#include "server/httpserver.h"
#include "server/pairingmanager.h"
#include "shared/constants.h"
#include "utils/appsettings.h"
#include "utils/helpers.h"
#include "utils/logsettings.h"
#include "utils/pairinginput.h"
#include "utils/singleinstanceguard.h"
#include "utils/systemtray.h"

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-avoid-c-arrays)
int main(int argc, char* argv[])
{
    utils::SingleInstanceGuard guard{shared::APP_NAME_BUDDY};
    if (!guard.tryToRun())
    {
        return EXIT_SUCCESS;
    }

    QApplication app{argc, argv};
    utils::LogSettings::getInstance().init("buddy.log");

    const utils::AppSettings app_settings{utils::getExecDir() + "settings.json"};
    utils::LogSettings::getInstance().setLoggingRules(app_settings.getLoggingRules());

    server::ClientIds      client_ids{utils::getExecDir() + "clients.json"};
    server::HttpServer     new_server{shared::API_VERSION, client_ids};
    server::PairingManager pairing_manager{client_ids};

    os::PcControl pc_control;

    const QIcon               icon{":/icons/app.ico"};
    const utils::SystemTray   tray{icon, shared::APP_NAME_BUDDY, pc_control};
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
    setupRoutes(new_server, pairing_manager, pc_control);

    client_ids.load();
    if (!new_server.startServer(app_settings.getPort(), ":/ssl/moondeck_cert.pem", ":/ssl/moondeck_key.pem"))
    {
        qFatal("Failed to start server!");
    }

    QGuiApplication::setQuitOnLastWindowClosed(false);
    return QCoreApplication::exec();
}
