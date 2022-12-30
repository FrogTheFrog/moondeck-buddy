#pragma once

// system/Qt includes
#include <QtHttpServer/QHttpServer>

// forward declaration
namespace server
{
class ClientIds;
}

//---------------------------------------------------------------------------------------------------------------------

namespace server
{
class HttpServer
{
    Q_DISABLE_COPY(HttpServer)

public:
    static QString getAuthorizationId(const QHttpServerRequest& request);

    explicit HttpServer(int api_version, ClientIds& client_ids);
    virtual ~HttpServer() = default;

    bool startServer(quint16 port, const QString& ssl_cert_file, const QString& ssl_key_file);

    int  getApiVersion() const;
    bool isAuthorized(const QHttpServerRequest& request) const;

    template<typename Rule = QHttpServerRouterRule, typename... Args>
    bool route(Args&&... args);

    template<typename ViewHandler>
    void afterRequest(ViewHandler&& viewHandler);

    void setMissingHandler(QHttpServer::MissingHandler handler);

private:
    int         m_api_version;
    ClientIds&  m_client_ids;
    QHttpServer m_server;
};

//---------------------------------------------------------------------------------------------------------------------

template<typename Rule, typename... Args>
bool HttpServer::route(Args&&... args)
{
    return m_server.route<Rule>(std::forward<Args>(args)...);
}

//---------------------------------------------------------------------------------------------------------------------

template<typename ViewHandler>
void HttpServer::afterRequest(ViewHandler&& viewHandler)
{
    return m_server.afterRequest(std::forward<ViewHandler>(viewHandler));
}
}  // namespace server
