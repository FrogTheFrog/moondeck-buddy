// system/Qt includes
#include <QApplication>
#include <QMainWindow>

// local includes
#include "os/commandproxy.h"
#include "os/pccontrol.h"
#include "os/steamproxy.h"
#include "server/jsonserver.h"
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
    utils::LogSettings::getInstance().init("output.log");

    const utils::AppSettings app_settings{utils::getExecDir() + "settings.json"};
    if (app_settings.isVerbose())
    {
        utils::LogSettings::getInstance().enableVerboseMode();
    }

    os::PcControl      pc_control{shared::APP_NAME_BUDDY};
    server::JsonServer server{shared::MSG_VERSION, utils::getExecDir() + "clients.json"};

    const os::CommandProxy command_proxy{pc_control};
    const os::SteamProxy   steam_proxy{pc_control};

    const QIcon               icon{":/icons/app.ico"};
    const utils::SystemTray   tray{icon, shared::APP_NAME_BUDDY, pc_control};
    const utils::PairingInput pairing;

    // Utils + app
    QObject::connect(&tray, &utils::SystemTray::signalQuitApp, &app, &QApplication::quit);

    // Server + pairing
    QObject::connect(&server, &server::JsonServer::signalRequestUserInputForPairing, &pairing,
                     &utils::PairingInput::slotRequestUserInputForPairing);
    QObject::connect(&server, &server::JsonServer::signalAbortPairing, &pairing,
                     &utils::PairingInput::slotAbortPairing);
    QObject::connect(&pairing, &utils::PairingInput::signalFinishPairing, &server,
                     &server::JsonServer::slotFinishPairing);
    QObject::connect(&pairing, &utils::PairingInput::signalPairingRejected, &server,
                     &server::JsonServer::slotPairingRejected);

    // Server + command proxy
    QObject::connect(&server, &server::JsonServer::signalCommandMessageReceived, &command_proxy,
                     &os::CommandProxy::slotHandleMessages);
    QObject::connect(&command_proxy, &os::CommandProxy::signalPcStateChanged, &server,
                     &server::JsonServer::slotPcStateChanged);

    // Server + steam proxy
    QObject::connect(&server, &server::JsonServer::signalSteamMessageReceived, &steam_proxy,
                     &os::SteamProxy::slotHandleMessages);
    QObject::connect(&steam_proxy, &os::SteamProxy::signalSendResponse, &server,
                     &server::JsonServer::slotSendSteamResponse);

    // HERE WE GO!!! (a.k.a. starting point)
    if (!server.startServer(app_settings.getPort(), ":/ssl/moondeck_cert.pem", ":/ssl/moondeck_key.pem"))
    {
        qFatal("Failed to start server!");
    }

    QGuiApplication::setQuitOnLastWindowClosed(false);
    return QCoreApplication::exec();
}
