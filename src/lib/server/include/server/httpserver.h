#pragma once

// system/Qt includes
#include <QtHttpServer/QHttpServer>

// local includes
#include "common/loggingcategories.h"
#include "json/json.h"

// forward declaration
namespace server
{
class ClientIds;
}

namespace server
{
namespace internal
{
template<typename T>
struct LambdaTraits;

template<typename ObjT, typename ReturnT, typename Arg>
struct LambdaTraits<ReturnT (ObjT::*)(Arg) const>
{
    using ArgType    = std::decay_t<Arg>;
    using ReturnType = std::decay_t<ReturnT>;
};

template<typename ObjT, typename ReturnT>
struct LambdaTraits<ReturnT (ObjT::*)() const>
{
    using ArgType    = void;
    using ReturnType = std::decay_t<ReturnT>;
};
}  // namespace internal

class RestServer final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(RestServer)

public:
    static QString getAuthorizationId(const QHttpServerRequest& request);

    explicit RestServer(int api_version, ClientIds& client_ids);
    ~RestServer() override = default;

    bool startServer(quint16 port, const QString& ssl_cert_file, const QString& ssl_key_file,
                     QSsl::SslProtocol protocol);

    int getApiVersion() const;

    template<typename Functor>
    void unauthenticatedHttpRoute(const QString& path_pattern, QHttpServerRequest::Methods method, Functor&& functor);
    template<typename Functor>
    void httpRoute(const QString& path_pattern, QHttpServerRequest::Methods method, Functor&& functor);

    template<typename Functor>
    void websocketRoute(const QString& path_pattern, Functor&& functor);

    template<typename ViewHandler>
    void afterRequest(ViewHandler&& view_handler);

private:
    bool isAuthorized(const QHttpServerRequest& request) const;

    template<typename T>
    static std::optional<T> fromHttpRequest(const QHttpServerRequest& request);
    template<typename T>
    static QHttpServerResponse toHttpResponse(const T& value);
    template<typename T>
    static QHttpServerResponse toHttpResponse(const std::variant<QHttpServerResponse::StatusCode, T>& value);
    template<typename Functor>
    auto httpRouteFunctorWrapper(bool secure, const Functor& functor);
    template<typename Functor>
    void httpRouteImpl(const QString& path_pattern, QHttpServerRequest::Methods method, Functor&& functor);

    void setupWebsocketSupport();

    int                        m_api_version;
    ClientIds&                 m_client_ids;
    QHttpServer                m_server;
    std::map<QString, QString> m_websocket_routes;
};

template<typename Functor>
void RestServer::unauthenticatedHttpRoute(const QString& path_pattern, const QHttpServerRequest::Methods method,
                                          Functor&& functor)
{
    httpRouteImpl(path_pattern, method, httpRouteFunctorWrapper(false, std::forward<Functor>(functor)));
}

template<typename Functor>
void RestServer::httpRoute(const QString& path_pattern, const QHttpServerRequest::Methods method, Functor&& functor)
{
    httpRouteImpl(path_pattern, method, httpRouteFunctorWrapper(true, std::forward<Functor>(functor)));
}

template<typename Functor>
void RestServer::websocketRoute(const QString& path_pattern, Functor&&)
{
    m_websocket_routes[path_pattern] = path_pattern;
}

template<typename ViewHandler>
void RestServer::afterRequest(ViewHandler&& view_handler)
{
    m_server.addAfterRequestHandler(&m_server, std::forward<ViewHandler>(view_handler));
}

template<typename T>
std::optional<T> RestServer::fromHttpRequest(const QHttpServerRequest& request)
{
    const auto& body{request.body()};
    if (body.isEmpty())
    {
        qCWarning(lc::server) << "Request is missing body!";
        return std::nullopt;
    }

    auto result{json::fromJson<T>(body)};
    if (!result)
    {
        qCWarning(lc::server) << "Failed to decode JSON data! Reason:\n" << result.error();
        return std::nullopt;
    }

    return std::move(result.value());
}

template<typename T>
QHttpServerResponse RestServer::toHttpResponse(const T& value)
{
    auto result{json::toJson<T>(value)};
    if (!result)
    {
        qCWarning(lc::server) << "Failed to encode JSON data! Reason:\n" << result.error();
        return QHttpServerResponse::StatusCode::InternalServerError;
    }

    return QHttpServerResponse{QByteArrayLiteral("application/json"), result.value().toUtf8()};
}

template<typename T>
QHttpServerResponse RestServer::toHttpResponse(const std::variant<QHttpServerResponse::StatusCode, T>& value)
{
    if (const auto* status_code{std::get<QHttpServerResponse::StatusCode>(&value)})
    {
        return *status_code;
    }

    return toHttpResponse<T>(std::get<T>(value));
}

template<typename Functor>
auto RestServer::httpRouteFunctorWrapper(const bool secure, const Functor& functor)
{
    using FunctorDecayed = std::decay_t<Functor>;
    using FunctorTraits  = internal::LambdaTraits<decltype(&FunctorDecayed::operator())>;
    using ArgType        = FunctorTraits::ArgType;
    using ReturnType     = FunctorTraits::ReturnType;

    const auto authenticator{[this, secure](const auto& http_request) -> std::optional<QHttpServerResponse>
                             {
                                 if (secure && !isAuthorized(http_request))
                                 {
                                     return QHttpServerResponse::StatusCode::Unauthorized;
                                 }

                                 return std::nullopt;
                             }};

    if constexpr (std::is_same_v<ArgType, void>)
    {
        return [authenticator, functor](const QHttpServerRequest& http_request)
        { return authenticator(http_request).value_or(toHttpResponse<ReturnType>(functor())); };
    }
    else if constexpr (std::is_same_v<ArgType, QString>)
    {
        return [authenticator, functor](const QString& arg, const QHttpServerRequest& http_request)
        { return authenticator(http_request).value_or(toHttpResponse<ReturnType>(functor(arg))); };
    }
    else if constexpr (std::is_same_v<ArgType, QHttpServerRequest>)
    {
        return [authenticator, functor](const QHttpServerRequest& http_request)
        { return authenticator(http_request).value_or(toHttpResponse<ReturnType>(functor(http_request))); };
    }
    else
    {
        return [authenticator, functor](const QHttpServerRequest& http_request) -> QHttpServerResponse
        {
            if (auto result{authenticator(http_request)})
            {
                return std::move(*result);
            }

            const auto request{fromHttpRequest<ArgType>(http_request)};
            if (!request)
            {
                return QHttpServerResponse::StatusCode::BadRequest;
            }

            return toHttpResponse<ReturnType>(functor(*request));
        };
    }
}

template<typename Functor>
void RestServer::httpRouteImpl(const QString& path_pattern, QHttpServerRequest::Methods method, Functor&& functor)
{
    static_assert(!std::is_member_function_pointer_v<Functor>, "Member function pointer are not allowed!");
    if (!m_server.route(path_pattern, method, std::forward<Functor>(functor)))
    {
        qFatal("Failed to route HTTP path %s!", qPrintable(path_pattern));
    }
}
}  // namespace server
