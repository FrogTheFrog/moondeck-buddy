// header file include
#include "jsonserver.h"

// system/Qt includes
#include <QFile>
#include <QSslKey>

// local includes
#include "msgs/converter.h"
#include "msgs/in/abortpairing.h"
#include "msgs/in/hello.h"
#include "msgs/in/login.h"
#include "msgs/in/pairstatus.h"
#include "msgs/in/pcstate.h"
#include "msgs/out/controltypetaken.h"
#include "msgs/out/invalidmessage.h"
#include "msgs/out/messageaccepted.h"
#include "msgs/out/notpaired.h"
#include "msgs/out/paired.h"
#include "msgs/out/pairing.h"
#include "msgs/out/pcstate.h"
#include "msgs/out/versionmismatch.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
constexpr uint DEFAULT_RESPONSE_TIMEOUT{10 * 1000 /*10 secs*/};

//---------------------------------------------------------------------------------------------------------------------

bool isIdUnique(const QUuid& socket_id, const std::map<QUuid, server::JsonServer::PendingSocket>& container,
                const std::unique_ptr<server::JsonSocket>& steam_socket,
                const std::unique_ptr<server::JsonSocket>& command_socket)
{
    if (container.contains(socket_id))
    {
        return false;
    }

    if (steam_socket != nullptr && steam_socket->getSocketId() == socket_id)
    {
        return false;
    }

    if (command_socket != nullptr && command_socket->getSocketId() == socket_id)
    {
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool handlePairQuery(std::unique_ptr<server::JsonSocket>& socket, const QJsonDocument& data,
                     const server::ClientIds& clients, const std::optional<QString>& pairing_id)
{
    const auto msg = converter::fromJson<msgs::in::PairStatus>(data);
    if (msg)
    {
        if (const auto* const pair_status = std::get_if<msgs::in::PairStatus>(&msg.value()); pair_status)
        {
            const bool is_paired{clients.containsId(pair_status->m_id)};
            const bool being_paired{pairing_id == pair_status->m_id};

            if (is_paired)
            {
                qDebug("Socket (id: %s) is asking about pair status (current status: PAIRED).",
                       qUtf8Printable(socket->getSocketId().toString()));
                socket->write(converter::toJson(msgs::out::Paired{}));
            }
            else if (being_paired)
            {
                qDebug("Socket (id: %s) is asking about pair status (current status: PAIRING).",
                       qUtf8Printable(socket->getSocketId().toString()));
                socket->write(converter::toJson(msgs::out::Pairing{}));
            }
            else
            {
                qDebug("Socket (id: %s) is asking about pair status (current status: NOT PAIRED).",
                       qUtf8Printable(socket->getSocketId().toString()));
                socket->write(converter::toJson(msgs::out::NotPaired{}));
            }
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------

bool handlePcStateQuery(std::unique_ptr<server::JsonSocket>& socket, const QJsonDocument& data, shared::PcState state)
{
    const auto msg = converter::fromJson<msgs::in::PcState>(data);
    if (msg)
    {
        if (const auto* const pair_status = std::get_if<msgs::in::PcState>(&msg.value()); pair_status)
        {
            qDebug("Socket (id: %s) is asking about PC state (current state: %s).",
                   qUtf8Printable(socket->getSocketId().toString()),
                   qUtf8Printable(QVariant::fromValue(state).toString()));
            socket->write(converter::toJson(msgs::out::PcState{state}));
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------

std::optional<QString> getPairingId(const std::optional<msgs::in::Pair>& pending_pairing)
{
    return pending_pairing ? std::make_optional(pending_pairing->m_id) : std::nullopt;
}

//---------------------------------------------------------------------------------------------------------------------

QString getPeerAddress(const QAbstractSocket& socket)
{
    const auto address = socket.peerAddress();
    return address.isNull() ? "unknown address" : address.toString();
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace server
{
JsonServer::JsonServer(int msg_version, QString client_filename)
    : m_msg_version{msg_version}
    , m_clients{std::move(client_filename)}
{
    m_clients.load();
    connect(&m_server, &QTcpServer::pendingConnectionAvailable, this, &JsonServer::slotHandleNewConnection);
    connect(&m_server, &QSslServer::errorOccurred, this,
            [](QSslSocket* socket, QAbstractSocket::SocketError error)
            {
                qDebug("Socket (%s) error: %s", qUtf8Printable(getPeerAddress(*socket)),
                       qUtf8Printable(QVariant::fromValue(error).toString()));
            });
}

//---------------------------------------------------------------------------------------------------------------------

bool JsonServer::startServer(quint16 port, const QString& ssl_cert_file, const QString& ssl_key_file)
{
    {
        QSslConfiguration ssl_config{QSslConfiguration::defaultConfiguration()};

        {
            QFile file{ssl_cert_file};
            if (!file.open(QFile::ReadOnly))
            {
                qWarning("Failed to load SSL certificate from %s!", qUtf8Printable(ssl_cert_file));
                return false;
            }

            const QSslCertificate cert{file.readAll()};
            ssl_config.setCaCertificates({});
            ssl_config.setLocalCertificate(cert);
        }

        {
            QFile file{ssl_key_file};
            if (!file.open(QFile::ReadOnly))
            {
                qWarning("Failed to load SSL key from %s!", qUtf8Printable(ssl_key_file));
                return false;
            }

            ssl_config.setPrivateKey(QSslKey{file.readAll(), QSsl::Rsa});
        }

        m_server.setSslConfiguration(ssl_config);
    }

    if (!m_server.listen(QHostAddress::Any, port))
    {
        qWarning("Server could not start listening at @%u", port);
        return false;
    }

    qInfo("Server started listening at @%u", port);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

void JsonServer::slotHandleNewConnection()
{
    if (!m_server.hasPendingConnections())
    {
        qWarning("No pending connections available!");
        return;
    }

    QAbstractSocket* socket = m_server.nextPendingConnection();
    if (socket == nullptr)
    {
        qFatal("Socket is nullptr!");
    }

    const QUuid socket_id{QUuid::createUuid()};
    if (!isIdUnique(socket_id, m_pending_sockets, m_steam_socket, m_command_socket))
    {
        qFatal("Socket id is not unique! WTF...");
    }

    auto json_socket{std::make_unique<JsonSocket>(socket_id, socket)};
    connect(json_socket.get(), &JsonSocket::signalDisconnected, this, &JsonServer::slotHandleDisconnect);
    connect(json_socket.get(), &JsonSocket::signalDataReceived, this, &JsonServer::slotHandleNewData);

    PendingSocket pending{std::move(json_socket)};
    pending.m_socket->setResponseTimeout(DEFAULT_RESPONSE_TIMEOUT);

    qDebug("New socket (id: %s) connected: %s", qUtf8Printable(socket_id.toString()),
           qUtf8Printable(getPeerAddress(*socket)));
    m_pending_sockets.emplace(socket_id, std::move(pending));
}

//---------------------------------------------------------------------------------------------------------------------

void JsonServer::slotHandleDisconnect(const QUuid& socket_id)
{
    if (m_command_socket && m_command_socket->getSocketId() == socket_id)
    {
        qDebug("Command socket (id: %s) has disconnected.", qUtf8Printable(socket_id.toString()));
        m_command_socket.reset();
    }
    else if (m_steam_socket && m_steam_socket->getSocketId() == socket_id)
    {
        qDebug("Steam socket (id: %s) has disconnected.", qUtf8Printable(socket_id.toString()));
        m_steam_socket.reset();
    }
    else
    {
        qDebug("Socket (id: %s) has disconnected.", qUtf8Printable(socket_id.toString()));
        m_pending_sockets.erase(socket_id);
    }
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void JsonServer::slotHandleNewData(const QUuid& socket_id, const QJsonDocument& data)
{
    if (m_command_socket && m_command_socket->getSocketId() == socket_id)
    {
        if (handlePairQuery(m_command_socket, data, m_clients, getPairingId(m_pending_pairing)))
        {
            return;
        }

        if (handlePcStateQuery(m_command_socket, data, m_pc_state))
        {
            return;
        }

        const auto msg = converter::fromJson<msgs::in::RestartPc, msgs::in::ShutdownPc>(data);
        if (!msg)
        {
            qWarning("Socket (id: %s) send an invalid data: %s", qUtf8Printable(socket_id.toString()),
                     qUtf8Printable(data.toJson(QJsonDocument::Compact)));
            m_command_socket->write(converter::toJson(msgs::out::InvalidMessage{}));
            return;
        }

        qDebug("Socket (id: %s) send a command message: %s", qUtf8Printable(socket_id.toString()),
               qUtf8Printable(data.toJson(QJsonDocument::Compact)));
        m_command_socket->write(converter::toJson(msgs::out::MessageAccepted{}));
        emit signalCommandMessageReceived(*msg);
    }
    else if (m_steam_socket && m_steam_socket->getSocketId() == socket_id)
    {
        if (handlePairQuery(m_steam_socket, data, m_clients, getPairingId(m_pending_pairing)))
        {
            return;
        }

        if (handlePcStateQuery(m_steam_socket, data, m_pc_state))
        {
            return;
        }

        const auto msg = converter::fromJson<msgs::in::LaunchApp, msgs::in::SteamStatus, msgs::in::CloseSteam>(data);
        if (!msg)
        {
            qWarning("Socket (id: %s) send an invalid data: %s", qUtf8Printable(socket_id.toString()),
                     qUtf8Printable(data.toJson(QJsonDocument::Compact)));
            m_steam_socket->write(converter::toJson(msgs::out::InvalidMessage{}));
            return;
        }

        qDebug("Socket (id: %s) send a steam message: %s", qUtf8Printable(socket_id.toString()),
               qUtf8Printable(data.toJson(QJsonDocument::Compact)));
        emit signalSteamMessageReceived(socket_id, *msg);
    }
    else
    {
        auto& socket{m_pending_sockets[socket_id].m_socket};
        Q_ASSERT(socket != nullptr);

        const auto hello_msg = converter::fromJson<msgs::in::Hello>(data);
        auto&      version_verified{m_pending_sockets[socket_id].m_version_verified};
        if (!version_verified || hello_msg)
        {
            if (!hello_msg)
            {
                qWarning("Socket (id: %s) send an invalid data: %s", qUtf8Printable(socket_id.toString()),
                         qUtf8Printable(data.toJson(QJsonDocument::Compact)));
                socket->write(converter::toJson(msgs::out::InvalidMessage{}));
                socket->close();
                return;
            }

            if (const auto* const hello = std::get_if<msgs::in::Hello>(&hello_msg.value()); hello)
            {
                if (hello->m_version != m_msg_version)
                {
                    qDebug("Socket (id: %s) has version mismatch. Expected %d, got %d.",
                           qUtf8Printable(socket_id.toString()), m_msg_version, hello->m_version);
                    socket->write(converter::toJson(msgs::out::VersionMismatch{m_msg_version}));
                    socket->close();
                    return;
                }

                qDebug("Socket (id: %s) says hello.", qUtf8Printable(socket_id.toString()));
                socket->write(converter::toJson(msgs::out::MessageAccepted{}));
                socket->setResponseTimeout(DEFAULT_RESPONSE_TIMEOUT);
                version_verified = true;
            }
            return;
        }

        if (handlePairQuery(socket, data, m_clients, getPairingId(m_pending_pairing)))
        {
            socket->setResponseTimeout(DEFAULT_RESPONSE_TIMEOUT);
            return;
        }

        if (handlePcStateQuery(socket, data, m_pc_state))
        {
            socket->setResponseTimeout(DEFAULT_RESPONSE_TIMEOUT);
            return;
        }

        const auto msg = converter::fromJson<msgs::in::Login, msgs::in::Pair, msgs::in::AbortPairing>(data);
        if (!msg)
        {
            qWarning("Socket (id: %s) send an invalid data: %s", qUtf8Printable(socket_id.toString()),
                     qUtf8Printable(data.toJson(QJsonDocument::Compact)));
            socket->write(converter::toJson(msgs::out::InvalidMessage{}));
            socket->close();
            return;
        }

        if (const auto* const login = std::get_if<msgs::in::Login>(&msg.value()); login)
        {
            if (!m_clients.containsId(login->m_id))
            {
                qDebug("Socket (id: %s) is not paired. Got unpaired id \"%s\".", qUtf8Printable(socket_id.toString()),
                       qUtf8Printable(login->m_id));
                socket->write(converter::toJson(msgs::out::NotPaired{}));
                socket->setResponseTimeout(DEFAULT_RESPONSE_TIMEOUT);
                return;
            }

            switch (login->m_constrol_type)
            {
                case msgs::in::Login::ControlType::Steam:
                {
                    if (m_steam_socket)
                    {
                        qDebug("Socket (id: %s) requested STEAM control which is already taken.",
                               qUtf8Printable(socket_id.toString()));
                        socket->write(converter::toJson(msgs::out::ControlTypeTaken{}));
                        socket->setResponseTimeout(DEFAULT_RESPONSE_TIMEOUT);
                        return;
                    }

                    qDebug("Socket (id: %s) got STEAM control.", qUtf8Printable(socket_id.toString()));
                    m_steam_socket = std::move(m_pending_sockets[socket_id].m_socket);
                    m_steam_socket->write(converter::toJson(msgs::out::MessageAccepted{}));
                }
                break;
                case msgs::in::Login::ControlType::Pc:
                {
                    if (m_command_socket)
                    {
                        qDebug("Socket (id: %s) requested PC control which is already taken.",
                               qUtf8Printable(socket_id.toString()));
                        socket->write(converter::toJson(msgs::out::ControlTypeTaken{}));
                        socket->setResponseTimeout(DEFAULT_RESPONSE_TIMEOUT);
                        return;
                    }

                    qDebug("Socket (id: %s) got PC control.", qUtf8Printable(socket_id.toString()));
                    m_command_socket = std::move(m_pending_sockets[socket_id].m_socket);
                    m_command_socket->write(converter::toJson(msgs::out::MessageAccepted{}));
                }
                break;
            }

            m_pending_sockets.erase(socket_id);
            return;
        }

        if (const auto* const pair = std::get_if<msgs::in::Pair>(&msg.value()); pair)
        {
            if (m_clients.containsId(pair->m_id))
            {
                qDebug("Socket (id: %s) is already paired.", qUtf8Printable(socket_id.toString()));
                socket->write(converter::toJson(msgs::out::Paired{}));
                socket->setResponseTimeout(DEFAULT_RESPONSE_TIMEOUT);
                return;
            }

            if (m_pending_pairing)
            {
                qDebug("Socket (id: %s) wants to pair, but pairing is already in progress.",
                       qUtf8Printable(socket_id.toString()));
                socket->write(converter::toJson(msgs::out::InvalidMessage{}));
                socket->setResponseTimeout(DEFAULT_RESPONSE_TIMEOUT);
                return;
            }

            qDebug("Socket (id: %s) started pairing with id: %s.", qUtf8Printable(socket_id.toString()),
                   qUtf8Printable(pair->m_id));
            m_pending_pairing = *pair;
            emit signalRequestUserInputForPairing(socket_id);
            socket->write(converter::toJson(msgs::out::MessageAccepted{}));
            socket->setResponseTimeout(DEFAULT_RESPONSE_TIMEOUT);
            return;
        }

        if (const auto* const abort_pairing = std::get_if<msgs::in::AbortPairing>(&msg.value()); abort_pairing)
        {
            if (!m_pending_pairing)
            {
                qDebug("Socket (id: %s) wants to abort pairing, but pairing is not in progress.",
                       qUtf8Printable(socket_id.toString()));
                socket->write(converter::toJson(msgs::out::MessageAccepted{}));
                socket->setResponseTimeout(DEFAULT_RESPONSE_TIMEOUT);
                return;
            }

            if (m_pending_pairing->m_id != abort_pairing->m_id)
            {
                qDebug("Socket (id: %s) wants to abort pairing, but the ID does not match.",
                       qUtf8Printable(socket_id.toString()));
                socket->write(converter::toJson(msgs::out::InvalidMessage{}));
                socket->setResponseTimeout(DEFAULT_RESPONSE_TIMEOUT);
                return;
            }

            qDebug("Socket (id: %s) has aborted pairing for id: %s.", qUtf8Printable(socket_id.toString()),
                   qUtf8Printable(abort_pairing->m_id));
            emit signalAbortPairing();
            m_pending_pairing = std::nullopt;
            socket->write(converter::toJson(msgs::out::MessageAccepted{}));
            socket->setResponseTimeout(DEFAULT_RESPONSE_TIMEOUT);
            return;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

void JsonServer::slotFinishPairing(uint pin)
{
    if (!m_pending_pairing)
    {
        qWarning("Pairing is not in progress!");
        return;
    }

    const auto pairing_data{*m_pending_pairing};
    m_pending_pairing = std::nullopt;

    const QString string_for_hashing{pairing_data.m_id + QString::number(pin)};
    if (string_for_hashing.toUtf8().toBase64() != pairing_data.m_hashed_id)
    {
        qWarning("Pairing code does not match.");
        return;
    }

    m_clients.addId(pairing_data.m_id);
    m_clients.save();
}

//---------------------------------------------------------------------------------------------------------------------

void JsonServer::slotPairingRejected()
{
    qDebug("Pairing was rejected.");
    m_pending_pairing = std::nullopt;
}

//---------------------------------------------------------------------------------------------------------------------

void JsonServer::slotSendSteamResponse(const QUuid& socket_id,
                                       const std::variant<msgs::out::SteamStatus, msgs::out::MessageAccepted>& msg)
{
    if (!m_steam_socket || m_steam_socket->getSocketId() != socket_id)
    {
        qDebug("Socket (id: %s) is no longer available to respond to.", qUtf8Printable(socket_id.toString()));
        return;
    }

    std::visit(
        [&](auto&& msg_v)
        {
            const auto data{converter::toJson(msg_v)};
            qDebug("Sending (id: %s) Steam response: %s", qUtf8Printable(socket_id.toString()),
                   qUtf8Printable(data.toJson(QJsonDocument::Compact)));
            m_steam_socket->write(data);
        },
        msg);
}

//---------------------------------------------------------------------------------------------------------------------

void JsonServer::slotPcStateChanged(shared::PcState state)
{
    m_pc_state = state;
}
}  // namespace server
