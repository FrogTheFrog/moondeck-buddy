#pragma once

// system/Qt includes
#include <QtHttpServer/QHttpServer>

// forward declaration
namespace server
{
class ClientIds;
}

namespace server
{
class RestServer
{
    Q_DISABLE_COPY(RestServer)

public:
    static QString getAuthorizationId(const QHttpServerRequest& request);

    explicit RestServer(int api_version, ClientIds& client_ids);
    virtual ~RestServer() = default;

    bool startServer(quint16 port, const QString& ssl_cert_file, const QString& ssl_key_file,
                     QSsl::SslProtocol protocol);

    int  getApiVersion() const;
    bool isAuthorized(const QHttpServerRequest& request) const;

    template<typename Functor>
    void httpRoute(const QString& path_pattern, QHttpServerRequest::Methods method, Functor&& functor);

    template<typename ViewHandler>
    void afterRequest(ViewHandler&& view_handler);

private:
    int         m_api_version;
    ClientIds&  m_client_ids;
    QHttpServer m_server;
};

template<typename Functor>
void RestServer::httpRoute(const QString& path_pattern, QHttpServerRequest::Methods method, Functor&& functor)
{
    static_assert(!std::is_member_function_pointer_v<Functor>, "Member function pointer are not allowed!");
    if (!m_server.route(path_pattern, method, std::forward<Functor>(functor)))
    {
        qFatal("Failed to route HTTP path %s!", qPrintable(path_pattern));
    }
}

template<typename ViewHandler>
void RestServer::afterRequest(ViewHandler&& view_handler)
{
    m_server.addAfterRequestHandler(&m_server, std::forward<ViewHandler>(view_handler));
}
}  // namespace server
