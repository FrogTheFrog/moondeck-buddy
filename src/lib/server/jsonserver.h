#pragma once

// system/Qt includes
#include <QTimer>
#include <QtNetwork/QSslServer>
#include <map>

// local includes
#include "clientids.h"
#include "jsonsocket.h"
#include "msgs/in/changeresolution.h"
#include "msgs/in/closesteam.h"
#include "msgs/in/launchapp.h"
#include "msgs/in/pair.h"
#include "msgs/in/restartpc.h"
#include "msgs/in/shutdownpc.h"
#include "msgs/in/steamstatus.h"
#include "msgs/out/messageaccepted.h"
#include "msgs/out/steamstatus.h"
#include "shared/enums.h"

//---------------------------------------------------------------------------------------------------------------------

namespace server
{
class JsonServer : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(JsonServer)

public:
    struct PendingSocket
    {
        std::unique_ptr<JsonSocket> m_socket{nullptr};
        bool                        m_version_verified{false};
    };

    explicit JsonServer(int msg_version, QString client_filename);
    ~JsonServer() override = default;

    bool startServer(quint16 port, const QString& ssl_cert_file, const QString& ssl_key_file);

signals:
    void signalCommandMessageReceived(
        const std::variant<msgs::in::RestartPc, msgs::in::ShutdownPc, msgs::in::CloseSteam>& msg);
    void signalSteamMessageReceived(const QUuid&                                    socket_id,
                                    const std::variant<msgs::in::LaunchApp, msgs::in::SteamStatus, msgs::in::CloseSteam,
                                                       msgs::in::ChangeResolution>& msg);
    void signalRequestUserInputForPairing(const QUuid& socket_id);
    void signalAbortPairing();
    void signalSteamClientConnectionStateChanged(bool connected);

private slots:
    void slotHandleNewConnection();
    void slotHandleDisconnect(const QUuid& socket_id);
    void slotHandleNewData(const QUuid& socket_id, const QJsonDocument& data);

public slots:
    void slotFinishPairing(uint pin);
    void slotPairingRejected();
    void slotSendSteamResponse(const QUuid&                                                            socket_id,
                               const std::variant<msgs::out::SteamStatus, msgs::out::MessageAccepted>& msg);
    void slotPcStateChanged(shared::PcState state);

private:
    int             m_msg_version;
    ClientIds       m_clients;
    shared::PcState m_pc_state{shared::PcState::Normal};

    std::unique_ptr<JsonSocket>    m_steam_socket{nullptr};
    std::unique_ptr<JsonSocket>    m_command_socket{nullptr};
    std::map<QUuid, PendingSocket> m_pending_sockets;
    std::optional<msgs::in::Pair>  m_pending_pairing{std::nullopt};

    QSslServer m_server;  // Server needs to be declared after sockets so that the sockets are destroyed first!
};
}  // namespace server
