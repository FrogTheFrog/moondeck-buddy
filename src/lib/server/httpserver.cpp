// header file include
#include "httpserver.h"

// system/Qt includes
#include <QFile>
#include <QSslKey>

// local includes
#include "clientids.h"
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace server
{
QString HttpServer::getAuthorizationId(const QHttpServerRequest& request)
{
    auto auth = request.value("authorization").simplified();

    if (auth.size() > 6 && auth.first(6).toLower() == "basic ")
    {
        auto token     = auth.sliced(6);
        auto client_id = QByteArray::fromBase64(token);

        if (!client_id.isEmpty())
        {
            return client_id;
        }
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------

HttpServer::HttpServer(int api_version, ClientIds& client_ids)
    : m_api_version{api_version}
    , m_client_ids{client_ids}
{
    Q_UNUSED(m_client_ids)
}

//---------------------------------------------------------------------------------------------------------------------

bool HttpServer::startServer(quint16 port, const QString& ssl_cert_file, const QString& ssl_key_file)
{
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

        m_server.sslSetup(QSslCertificate{cert_file.readAll()}, QSslKey{key_file.readAll(), QSsl::Rsa});
    }

    if (!m_server.listen(QHostAddress::Any, port))
    {
        qCWarning(lc::server) << "Server could not start listening at port" << port;
        return false;
    }

    qCInfo(lc::server) << "Server started listening at port" << port;
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

int HttpServer::getApiVersion() const
{
    return m_api_version;
}

//---------------------------------------------------------------------------------------------------------------------

bool HttpServer::isAuthorized(const QHttpServerRequest& request) const
{
    return m_client_ids.containsId(getAuthorizationId(request));
}

//---------------------------------------------------------------------------------------------------------------------

void HttpServer::setMissingHandler(QHttpServer::MissingHandler handler)
{
    return m_server.setMissingHandler(std::move(handler));
}
}  // namespace server