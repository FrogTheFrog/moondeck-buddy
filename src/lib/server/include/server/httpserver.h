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
class HttpServer
{
    Q_DISABLE_COPY(HttpServer)

public:
    static QString getAuthorizationId(const QHttpServerRequest& request);

    explicit HttpServer(int api_version, ClientIds& client_ids);
    virtual ~HttpServer() = default;

    bool startServer(quint16 port, const QString& ssl_cert_file, const QString& ssl_key_file,
                     QSsl::SslProtocol protocol);

    int  getApiVersion() const;
    bool isAuthorized(const QHttpServerRequest& request) const;

    template<typename Functor>
    bool route(const QString& path_pattern, QHttpServerRequest::Methods method, Functor&& functor);

    template<typename ViewHandler>
    void afterRequest(ViewHandler&& view_handler);

private:
    int         m_api_version;
    ClientIds&  m_client_ids;
    QHttpServer m_server;
};

template<typename Functor>
bool HttpServer::route(const QString& path_pattern, QHttpServerRequest::Methods method, Functor&& functor)
{
    static_assert(!std::is_member_function_pointer_v<Functor>, "Member function pointer are not allowed!");
    return m_server.route(path_pattern, method, std::forward<Functor>(functor));
}

template<typename ViewHandler>
void HttpServer::afterRequest(ViewHandler&& view_handler)
{
    return m_server.addAfterRequestHandler(&m_server, std::forward<ViewHandler>(view_handler));
}
}  // namespace server
