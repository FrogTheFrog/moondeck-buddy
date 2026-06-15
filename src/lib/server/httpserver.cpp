// header file include
#include "server/httpserver.h"

// system/Qt includes
#include <QFile>
#include <QSslKey>
#include <QSslServer>

// local includes
#include "common/enums.h"
#include "common/loggingcategories.h"
#include "server/clientids.h"

namespace server
{
QString RestServer::getAuthorizationId(const QHttpServerRequest& request)
{
    const auto auth = request.value("authorization").simplified();

    if (constexpr int id_start_index{6};
        auth.size() > id_start_index && auth.first(id_start_index).toLower() == "basic ")
    {
        const auto token = auth.sliced(id_start_index);
        if (auto client_id = QByteArray::fromBase64(token); !client_id.isEmpty())
        {
            return client_id;
        }
    }

    return {};
}

RestServer::RestServer(const int api_version, ClientIds& client_ids)
    : m_api_version{api_version}
    , m_client_ids{client_ids}
{
    setupWebsocketSupport();
}

bool RestServer::startServer(const quint16 port, const QString& ssl_cert_file, const QString& ssl_key_file,
                             const QSsl::SslProtocol protocol)
{
    auto ssl_server = std::make_unique<QSslServer>();
    {
        QFile cert_file{ssl_cert_file};
        if (!cert_file.open(QFile::ReadOnly))
        {
            qCWarning(lc::server) << "Failed to load SSL certificate from" << ssl_cert_file;
            return false;
        }

        QFile key_file{ssl_key_file};
        if (!key_file.open(QFile::ReadOnly))
        {
            qCWarning(lc::server) << "Failed to load SSL key from" << ssl_key_file;
            return false;
        }

        QSslConfiguration ssl_conf{QSslConfiguration::defaultConfiguration()};
        ssl_conf.setLocalCertificate(QSslCertificate{cert_file.readAll()});
        ssl_conf.setPrivateKey(QSslKey{key_file.readAll(), QSsl::Rsa});
        ssl_conf.setProtocol(protocol);

        ssl_server->setSslConfiguration(ssl_conf);
    }

    if (!ssl_server->listen(QHostAddress::Any, port))
    {
        qCWarning(lc::server) << "Server could not start listening at port" << port;
        return false;
    }

    if (!m_server.bind(ssl_server.get()))
    {
        qCWarning(lc::server) << "Failed to bind the ssl server!";
        return false;
    }
    ssl_server.release();  // m_server has taken over the ownership!

    qCInfo(lc::server) << "Server started listening at port" << port;
    return true;
}

int RestServer::getApiVersion() const
{
    return m_api_version;
}

bool RestServer::isAuthorized(const QHttpServerRequest& request) const
{
    return m_client_ids.containsId(getAuthorizationId(request));
}

void RestServer::setupWebsocketSupport()
{
    m_server.addWebSocketUpgradeVerifier(&m_server,
                                         [this](const QHttpServerRequest& request)
                                         {
                                             if (!isAuthorized(request))
                                             {
                                                 constexpr auto status{QHttpServerResponse::StatusCode::Unauthorized};
                                                 return QHttpServerWebSocketUpgradeResponse::deny(
                                                     static_cast<int>(status), enums::qEnumToString(status).toUtf8());
                                             }

                                             if (!m_websocket_routes.contains(request.url().path()))
                                             {
                                                 constexpr auto status{QHttpServerResponse::StatusCode::NotFound};
                                                 return QHttpServerWebSocketUpgradeResponse::deny(
                                                     static_cast<int>(status), enums::qEnumToString(status).toUtf8());
                                             }

                                             qCDebug(lc::server) << "New WebSocket between:" << request.remoteAddress()
                                                                 << "<->" << request.url();
                                             return QHttpServerWebSocketUpgradeResponse::accept();
                                         });

    connect(&m_server, &QHttpServer::newWebSocketConnection, this,
            [this]()
            {
                while (m_server.hasPendingWebSocketConnections())
                {
                    QWebSocket* socket{m_server.nextPendingWebSocketConnection().release()};
                    socket->setParent(&m_server);

                    connect(socket, &QWebSocket::textMessageReceived, socket,
                            [socket](const QString& message)
                            {
                                qInfo() << "  received:" << message;
                                socket->sendTextMessage("echo: " + message);
                            });
                    connect(socket, &QWebSocket::errorOccurred, socket, [](const QAbstractSocket::SocketError error)
                            { qCWarning(lc::server) << "WebSocket error occurred:" << error; });
                    connect(socket, &QWebSocket::disconnected, socket, &QWebSocket::deleteLater);
                }
            });
}
}  // namespace server