#pragma once

// system/Qt includes
#include <QtHttpServer/QHttpServer>
#include <QtWebSockets/QWebSocket>
#include <typeindex>

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

template<typename ObjT, typename ReturnT, typename... ArgT>
struct LambdaTraits<ReturnT (ObjT::*)(ArgT...) const>
{
private:
    template<size_t Index, typename Tuple>
    struct TupleElement;

    template<size_t Index, typename Tuple>
        requires(std::tuple_size_v<Tuple> > Index)
    struct TupleElement<Index, Tuple>
    {
        using Type = std::tuple_element_t<Index, Tuple>;
    };

    template<size_t, typename>
    struct TupleElement
    {
        using Type = void;
    };

public:
    using ArgTuple   = std::tuple<ArgT...>;
    using ReturnType = ReturnT;

    template<size_t Index>
    using ArgType = TupleElement<Index, ArgTuple>::Type;
};

template<typename T>
std::optional<T> fromJson(const QString& value)
{
    auto result{json::fromJson<T>(value)};
    if (!result)
    {
        qCWarning(lc::server).noquote().nospace() << "Failed to decode JSON data! Reason:\n" << result.error();
        return std::nullopt;
    }

    return std::move(result.value());
}

template<typename T>
std::optional<QString> toJson(const T& value)
{
    auto result{json::toJson<T>(value)};
    if (!result)
    {
        qCWarning(lc::server).noquote().nospace() << "Failed to encode JSON data! Reason:\n" << result.error();
        return std::nullopt;
    }

    return std::move(result.value());
}

}  // namespace internal

class WebSocket final : public QObject
{
    Q_OBJECT

public:
    explicit WebSocket(QWebSocket* socket);
    ~WebSocket() override;

    static QString getRoutePath(const QHttpServerRequest& request);
    static QString getRoutePath(const QWebSocket& socket);
    QString        getRoutePath() const;
    QHostAddress   peerAddress() const;

    const QString& getIdString() const;
    bool           isConnected() const;

    template<typename T>
    void sendJson(const T& value);
    void close(QWebSocketProtocol::CloseCode code = QWebSocketProtocol::CloseCodeNormal);

    template<typename T, typename... Args>
    T& getOrCreateStoredData(Args&&... args);
    template<typename T>
    void clearStoredData();
    template<typename T, typename... Args>
    T& createOrOverrideStoredData(Args&&... args);

private:
    QWebSocket* m_socket;
    QString     m_id_string;

    using AnyPtr = std::unique_ptr<void, void (*)(void*)>;
    std::map<std::type_index, AnyPtr> m_stored_data;
};

template<typename T>
void WebSocket::sendJson(const T& value)
{
    if (!isConnected())
    {
        qCDebug(lc::server).noquote() << "Socket is already closed! Discarding leftover data...";
        return;
    }

    if (const auto json_string{internal::toJson(value)})
    {
        qCDebug(lc::server).noquote() << getIdString() << "sending:" << *json_string;
        m_socket->sendTextMessage(*json_string);
    }
    else
    {
        m_socket->close(QWebSocketProtocol::CloseCodeBadOperation);
    }
}

template<typename T, typename... Args>
T& WebSocket::getOrCreateStoredData(Args&&... args)
{
    using DataType = std::decay_t<T>;
    const auto index{std::type_index(typeid(DataType))};

    auto data_it{m_stored_data.find(index)};
    if (data_it == std::end(m_stored_data))
    {
        data_it = m_stored_data
                      .emplace(index, AnyPtr{new DataType{std::forward<Args>(args)...},
                                             [](void* ptr) { delete static_cast<DataType*>(ptr); }})
                      .first;
        if (data_it == std::end(m_stored_data))
        {
            qFatal("Failed to instantiate stored data type %s!", index.name());
        }
    }

    T* data_ptr{static_cast<DataType*>(data_it->second.get())};
    if (data_ptr == nullptr)
    {
        qFatal("Failed to cast to data type %s!", index.name());
    }

    return *data_ptr;
}

template<typename T>
void WebSocket::clearStoredData()
{
    using DataType = std::decay_t<T>;
    const auto index{std::type_index(typeid(DataType))};

    m_stored_data.erase(index);
}

template<typename T, typename... Args>
T& WebSocket::createOrOverrideStoredData(Args&&... args)
{
    clearStoredData<T>();
    return getOrCreateStoredData<T, Args...>(std::forward<Args>(args)...);
}

class RestServer final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(RestServer)

public:
    explicit RestServer(int api_version, ClientIds& client_ids);
    ~RestServer() override = default;

    bool startServer(quint16 port, const QString& ssl_cert_file, const QString& ssl_key_file,
                     QSsl::SslProtocol protocol);

    int getApiVersion() const;

    template<typename Functor>
    void unauthenticatedHttpRoute(const QString& path_pattern, QHttpServerRequest::Methods method, Functor&& functor);
    template<typename Functor>
    void httpRoute(const QString& path_pattern, QHttpServerRequest::Methods method, Functor&& functor);

    template<typename InitializerOrResponder>
    void webSocketRoute(const QString& path_pattern, InitializerOrResponder&& functor);
    template<typename Initializer, typename Responder>
    void webSocketRoute(const QString& path_pattern, Initializer&& initializer_functor, Responder&& responder_functor);

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
    auto httpRouteFunctorWrapper(bool secure, Functor&& functor);
    template<typename Functor>
    void httpRouteImpl(const QString& path_pattern, QHttpServerRequest::Methods method, Functor&& functor);

    using InitializerFunctor = std::function<void(WebSocket&)>;
    using ResponderFunctor   = std::function<void(WebSocket&, const QString&)>;
    using WebSocketHandlers  = std::pair<InitializerFunctor, ResponderFunctor>;

    void setupWebSocketHandling();
    template<typename Functor>
    InitializerFunctor webSocketInitializerFunctorWrapper(Functor&& functor);
    template<typename Functor>
    ResponderFunctor webSocketResponderFunctorWrapper(Functor&& functor);

    int                                  m_api_version;
    ClientIds&                           m_client_ids;
    QHttpServer                          m_server;
    std::map<QString, WebSocketHandlers> m_websocket_routes;
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

template<typename InitializerOrResponder>
void RestServer::webSocketRoute(const QString& path_pattern, InitializerOrResponder&& functor)
{
    using FunctorTraits = internal::LambdaTraits<decltype(&std::decay_t<InitializerOrResponder>::operator())>;
    using FirstArgType  = FunctorTraits::template ArgType<0>;
    using SecondArgType = FunctorTraits::template ArgType<1>;

    if constexpr ((std::is_same_v<FirstArgType, void> && std::is_same_v<SecondArgType, void>)
                  || (std::is_same_v<FirstArgType, WebSocket&> && std::is_same_v<SecondArgType, void>))
    {
        webSocketRoute(path_pattern, std::forward<InitializerOrResponder>(functor), std::nullopt);
    }
    else
    {
        webSocketRoute(path_pattern, std::nullopt, std::forward<InitializerOrResponder>(functor));
    }
}

template<typename Initializer, typename Responder>
void RestServer::webSocketRoute(const QString& path_pattern, Initializer&& initializer_functor,
                                Responder&& responder_functor)
{
    if (m_websocket_routes.contains(path_pattern))
    {
        qFatal("WebSocket route %s already exists!", qPrintable(path_pattern));
    }

    m_websocket_routes[path_pattern] = {
        webSocketInitializerFunctorWrapper(std::forward<Initializer>(initializer_functor)),
        webSocketResponderFunctorWrapper(std::forward<Responder>(responder_functor))};
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

    return internal::fromJson<T>(body);
}

template<typename T>
QHttpServerResponse RestServer::toHttpResponse(const T& value)
{
    auto result{internal::toJson<T>(value)};
    if (!result)
    {
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
auto RestServer::httpRouteFunctorWrapper(const bool secure, Functor&& functor)
{
    using FunctorTraits = internal::LambdaTraits<decltype(&std::decay_t<Functor>::operator())>;
    using FirstArgType  = std::decay_t<typename FunctorTraits::template ArgType<0>>;
    using ReturnType    = std::decay_t<typename FunctorTraits::ReturnType>;

    const auto authenticator{[this, secure](const auto& http_request) -> std::optional<QHttpServerResponse>
                             {
                                 if (secure && !isAuthorized(http_request))
                                 {
                                     return QHttpServerResponse::StatusCode::Unauthorized;
                                 }

                                 return std::nullopt;
                             }};

    if constexpr (std::is_same_v<FirstArgType, void>)
    {
        return [authenticator, functor = std::forward<Functor>(functor)](const QHttpServerRequest& http_request)
        { return authenticator(http_request).value_or(toHttpResponse<ReturnType>(functor())); };
    }
    else if constexpr (std::is_same_v<FirstArgType, QString>)
    {
        return [authenticator, functor = std::forward<Functor>(functor)](const QString&            arg,
                                                                         const QHttpServerRequest& http_request)
        { return authenticator(http_request).value_or(toHttpResponse<ReturnType>(functor(arg))); };
    }
    else if constexpr (std::is_same_v<FirstArgType, QHttpServerRequest>)
    {
        return [authenticator, functor = std::forward<Functor>(functor)](const QHttpServerRequest& http_request)
        { return authenticator(http_request).value_or(toHttpResponse<ReturnType>(functor(http_request))); };
    }
    else
    {
        return [authenticator,
                functor = std::forward<Functor>(functor)](const QHttpServerRequest& http_request) -> QHttpServerResponse
        {
            if (auto result{authenticator(http_request)})
            {
                return std::move(*result);
            }

            const auto request{fromHttpRequest<FirstArgType>(http_request)};
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

template<typename Functor>
RestServer::InitializerFunctor RestServer::webSocketInitializerFunctorWrapper(Functor&& functor)
{
    return [functor = std::forward<Functor>(functor)](WebSocket& web_socket)
    {
        if constexpr (std::is_same_v<std::decay_t<Functor>, std::nullopt_t>)
        {
            // No initializer set, which is fine...
            Q_UNUSED(web_socket);
        }
        else
        {
            using FunctorTraits = internal::LambdaTraits<decltype(&std::decay_t<Functor>::operator())>;
            using FirstArgType  = FunctorTraits::template ArgType<0>;

            if constexpr (std::is_same_v<FirstArgType, WebSocket&>)
            {
                functor(web_socket);
            }
            else
            {
                functor();
            }
        }
    };
}

template<typename Functor>
RestServer::ResponderFunctor RestServer::webSocketResponderFunctorWrapper(Functor&& functor)
{
    return [functor = std::forward<Functor>(functor)](WebSocket& web_socket, const QString& input)
    {
        auto close_socket_on_conversion_error{
            qScopeGuard([&web_socket]() { web_socket.close(QWebSocketProtocol::CloseCodeBadOperation); })};
        if constexpr (std::is_same_v<std::decay_t<Functor>, std::nullopt_t>)
        {
            qCWarning(lc::server) << "WebSocket route does not support Req/Resp pattern!";
            Q_UNUSED(input);
        }
        else
        {
            using FunctorTraits = internal::LambdaTraits<decltype(&std::decay_t<Functor>::operator())>;
            using FirstArgType  = FunctorTraits::template ArgType<0>;
            using SecondArgType = FunctorTraits::template ArgType<1>;
            using ReturnType    = std::decay_t<typename FunctorTraits::ReturnType>;

            if constexpr (std::is_same_v<FirstArgType, WebSocket&>)
            {
                if (auto parsed_input{internal::fromJson<std::decay_t<SecondArgType>>(input)})
                {
                    close_socket_on_conversion_error.dismiss();
                    web_socket.sendJson<ReturnType>(functor(web_socket, std::move(*parsed_input)));
                }
            }
            else
            {
                if (auto parsed_input{internal::fromJson<std::decay_t<FirstArgType>>(input)})
                {
                    close_socket_on_conversion_error.dismiss();
                    web_socket.sendJson<ReturnType>(functor(std::move(*parsed_input)));
                }
            }
        }
    };
}
}  // namespace server
