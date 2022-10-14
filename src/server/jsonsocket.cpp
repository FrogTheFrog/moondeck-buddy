// header file include
#include "jsonsocket.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const qsizetype DEFAULT_BUFFER_SIZE{512};
}

//---------------------------------------------------------------------------------------------------------------------

namespace server
{
JsonSocket::JsonSocket(QUuid socket_id, QAbstractSocket* socket)
    : m_socket_id{socket_id}
    , m_socket{socket}
    , m_protocol{DEFAULT_BUFFER_SIZE}
{
    if (m_socket == nullptr)
    {
        qFatal("Socket cannot be nullptr!");
    }

    // Auto close timer
    connect(&m_response_timeout, &QTimer::timeout, this, [this]() { close(); });
    m_response_timeout.setSingleShot(true);

    // Socket
    connect(m_socket, &QAbstractSocket::disconnected, this, [this]() { close(); });
    connect(m_socket, &QAbstractSocket::readyRead, this,
            [this]() { emit m_protocol.signalRawDataReceived(m_socket->readAll()); });

    // Protocol
    connect(&m_protocol, &CobsJsonProtocol::signalJsonDataReceived, this,
            [this](const auto& data)
            {
                m_response_timeout.stop();
                emit signalDataReceived(m_socket_id, data);
            });
    connect(&m_protocol, &CobsJsonProtocol::signalSendRawData, this,
            [this](const auto& data)
            {
                if (isOpen())
                {
                    m_socket->write(data);
                }
            });
}

//---------------------------------------------------------------------------------------------------------------------

JsonSocket::~JsonSocket()
{
    close();
}

//---------------------------------------------------------------------------------------------------------------------

const QUuid& JsonSocket::getSocketId() const
{
    return m_socket_id;
}

//---------------------------------------------------------------------------------------------------------------------

bool JsonSocket::isOpen() const
{
    return m_socket != nullptr;
}

//---------------------------------------------------------------------------------------------------------------------

void JsonSocket::setResponseTimeout(uint msecs)
{
    m_response_timeout.start(static_cast<int>(msecs));
}

//---------------------------------------------------------------------------------------------------------------------

void JsonSocket::write(const QJsonDocument& data)
{
    if (isOpen())
    {
        emit m_protocol.signalSendJsonData(data);
    }
}

//---------------------------------------------------------------------------------------------------------------------

void JsonSocket::close()
{
    if (isOpen())
    {
        m_response_timeout.stop();

        // Disconnecting ourselves from the socket
        disconnect(m_socket, nullptr, this, nullptr);

        if (m_socket->state() != QAbstractSocket::UnconnectedState)
        {
            connect(m_socket, &QAbstractSocket::disconnected, m_socket, &QAbstractSocket::deleteLater);
            m_socket->disconnectFromHost();
        }
        else
        {
            m_socket->deleteLater();
        }

        m_socket = nullptr;
        emit signalDisconnected(m_socket_id);
    }
}
}  // namespace server
