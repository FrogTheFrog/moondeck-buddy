// system/Qt includes
#include <QApplication>
#include <QDir>
#include <QFile>

// local includes
#include "os/commandproxy.h"
#include "os/pccontrol.h"
#include "os/steamproxy.h"
#include "server/jsonserver.h"
#include "utils/appsettings.h"
#include "utils/pairinginput.h"
#include "utils/singleinstanceguard.h"
#include "utils/systemtray.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const QString APP_NAME{"SteamDeckBuddy"};
constexpr int MSG_VERSION{1};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bool VERBOSE_LOGGING{false};

//---------------------------------------------------------------------------------------------------------------------

const QString& getExecDir()
{
    static const QString dir{QCoreApplication::applicationDirPath() + "/"};
    return dir;
}

//---------------------------------------------------------------------------------------------------------------------

const QString& getOutputFilename()
{
    static const QString filename{getExecDir() + "output.log"};
    return filename;
}

//---------------------------------------------------------------------------------------------------------------------

void removeLogFile()
{
    QFile file(getOutputFilename());
    if (file.exists())
    {
        file.remove();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void messageHandler(QtMsgType type, const QMessageLogContext& /*unused*/, const QString& msg)
{
    QString formatted_msg;
    {
        const QString now{QDateTime::currentDateTime().toString("hh:mm:ss.zzz")};
        QTextStream   stream(&formatted_msg);
        switch (type)
        {
            case QtDebugMsg:
                if (!VERBOSE_LOGGING)
                {
                    return;
                }

                stream << "[" << now << "] DEBUG:    " << msg;
                break;
            case QtInfoMsg:
                stream << "[" << now << "] INFO:     " << msg;
                break;
            case QtWarningMsg:
                stream << "[" << now << "] WARNING:  " << msg;
                break;
            case QtCriticalMsg:
                stream << "[" << now << "] CRITICAL: " << msg;
                break;
            case QtFatalMsg:
                stream << "[" << now << "] FATAL:    " << msg;
                break;
        }
    }

    {
        QTextStream stream(stdout);
        stream << formatted_msg << Qt::endl;
    }

    QFile file(getOutputFilename());
    if (file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QTextStream stream(&file);
        stream << formatted_msg << Qt::endl;
    }
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-avoid-c-arrays)
int main(int argc, char* argv[])
{
    utils::SingleInstanceGuard guard{APP_NAME};
    if (!guard.tryToRun())
    {
        return EXIT_SUCCESS;
    }

    QApplication app{argc, argv};
    removeLogFile();
    qInstallMessageHandler(messageHandler);

    const utils::AppSettings app_settings{getExecDir() + "settings.json"};
    if (app_settings.isVerbose())
    {
        VERBOSE_LOGGING = true;
    }

    os::PcControl      pc_control{APP_NAME};
    server::JsonServer server{MSG_VERSION, getExecDir() + "clients.json"};

    const os::CommandProxy command_proxy{pc_control};
    const os::SteamProxy   steam_proxy{pc_control};

    const QIcon               icon{":/icons/app.ico"};
    const utils::SystemTray   tray{icon, APP_NAME, pc_control};
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

    if (!server.startServer(app_settings.getPort(), ":/ssl/moondeck_cert.pem", ":/ssl/moondeck_key.pem"))
    {
        qFatal("Failed to start server!");
    }

    QGuiApplication::setQuitOnLastWindowClosed(false);
    return QCoreApplication::exec();
}
