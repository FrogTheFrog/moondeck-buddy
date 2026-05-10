// header file include
#include "routing.h"

// local includes
#include "common/loggingcategories.h"
#include "os/networkinfo.h"
#include "server/httpserver.h"
#include "json/json.h"

namespace
{
template<typename T>
std::optional<T> fromRequest(const QHttpServerRequest& request)
{
    const auto& body{request.body()};
    if (body.isEmpty())
    {
        qCWarning(lc::buddyMain) << "Request is missing body!";
        return std::nullopt;
    }

    auto result{json::fromJson<T>(body)};
    if (!result)
    {
        qCWarning(lc::buddyMain) << "Failed to decode JSON data! Reason:\n" << result.error();
        return std::nullopt;
    }

    return std::move(result.value());
}

template<typename T>
QHttpServerResponse toResponse(const T& value)
{
    auto result{json::toJson<T, {.skip_null_members = false}>(value)};
    if (!result)
    {
        qCWarning(lc::buddyMain) << "Failed to encode JSON data! Reason:\n" << result.error();
        return QHttpServerResponse::StatusCode::InternalServerError;
    }

    return QHttpServerResponse{QByteArrayLiteral("application/json"), result.value().toUtf8()};
}

template<typename T>
QHttpServerResponse toResponse(const std::variant<QHttpServerResponse::StatusCode, T>& value)
{
    if (const auto* status_code{std::get<QHttpServerResponse::StatusCode>(&value)})
    {
        return *status_code;
    }

    return toResponse<T>(std::get<T>(value));
}

//----------------------------------------------------------------------------------------------------------------------

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

//----------------------------------------------------------------------------------------------------------------------

template<typename FunctorT>
auto reqRespFunctorWrapper(const server::HttpServer* authentication_server, const FunctorT& functor)
{
    using Functor       = std::decay_t<FunctorT>;
    using FunctorTraits = LambdaTraits<decltype(&Functor::operator())>;
    using ArgType       = FunctorTraits::ArgType;
    using ReturnType    = FunctorTraits::ReturnType;

    const auto authenticator{[authentication_server](const auto& http_request) -> std::optional<QHttpServerResponse>
                             {
                                 if (authentication_server && !authentication_server->isAuthorized(http_request))
                                 {
                                     return QHttpServerResponse::StatusCode::Unauthorized;
                                 }

                                 return std::nullopt;
                             }};

    if constexpr (std::is_same_v<ArgType, void>)
    {
        return [authenticator, functor](const QHttpServerRequest& http_request)
        { return authenticator(http_request).value_or(toResponse<ReturnType>(functor())); };
    }
    else if constexpr (std::is_same_v<ArgType, QString>)
    {
        return [authenticator, functor](const QString& arg, const QHttpServerRequest& http_request)
        { return authenticator(http_request).value_or(toResponse<ReturnType>(functor(arg))); };
    }
    else if constexpr (std::is_same_v<ArgType, QHttpServerRequest>)
    {
        return [authenticator, functor](const QHttpServerRequest& http_request)
        { return authenticator(http_request).value_or(toResponse<ReturnType>(functor(http_request))); };
    }
    else
    {
        return [authenticator, functor](const QHttpServerRequest& http_request) -> QHttpServerResponse
        {
            if (const auto result{authenticator(http_request)})
            {
                return result->statusCode();
            }

            const auto request{fromRequest<ArgType>(http_request)};
            if (!request)
            {
                return QHttpServerResponse::StatusCode::BadRequest;
            }

            return toResponse<ReturnType>(functor(*request));
        };
    }
}

template<typename FunctorT>
void reqRespRouter(server::HttpServer& server, const QString& path_pattern, const QHttpServerRequest::Methods method,
                   const bool secure, const FunctorT& functor)
{
    server.route(path_pattern, method, reqRespFunctorWrapper(secure ? &server : nullptr, functor));
}

template<typename FunctorT>
void openReqResp(server::HttpServer& server, const QString& path_pattern, const QHttpServerRequest::Methods method,
                 FunctorT&& functor)
{
    reqRespRouter(server, path_pattern, method, false, std::forward<FunctorT>(functor));
}

template<typename FunctorT>
void secureReqResp(server::HttpServer& server, const QString& path_pattern, const QHttpServerRequest::Methods method,
                   FunctorT&& functor)
{
    reqRespRouter(server, path_pattern, method, true, std::forward<FunctorT>(functor));
}
}  // namespace

namespace http_api
{
struct ResultResponse
{
    bool m_result;
};

//----------------------------------------------------------------------------------------------------------------------

struct VersionResponse
{
    int m_version;
};

void apiVersion(server::HttpServer& server)
{
    openReqResp(server, "/apiVersion", QHttpServerRequest::Method::Get,
                [&server]() { return VersionResponse{.m_version = server.getApiVersion()}; });
}

//----------------------------------------------------------------------------------------------------------------------

struct PairingStateResponse
{
    Q_GADGET

public:
    enum class PairingState
    {
        Paired,
        Pairing,
        NotPaired
    };
    Q_ENUM(PairingState)

    PairingState m_state;
};

void pairingState(server::HttpServer& server, server::PairingManager& pairing_manager)
{
    openReqResp(server, "/pairingState/<arg>", QHttpServerRequest::Method::Get,
                [&pairing_manager](const QString& user_id)
                {
                    using enum PairingStateResponse::PairingState;

                    const auto state{pairing_manager.isPaired(user_id)    ? Paired
                                     : pairing_manager.isPairing(user_id) ? Pairing
                                                                          : NotPaired};
                    return PairingStateResponse{.m_state = state};
                });
}

//----------------------------------------------------------------------------------------------------------------------

struct PairRequest
{
    QString m_id;
    QString m_hashed_id;
};

void pair(server::HttpServer& server, server::PairingManager& pairing_manager)
{
    openReqResp(server, "/pair", QHttpServerRequest::Method::Post,
                [&pairing_manager](const PairRequest& request)
                {
                    const bool result{pairing_manager.startPairing(request.m_id, request.m_hashed_id)};
                    return ResultResponse{.m_result = result};
                });
}

//----------------------------------------------------------------------------------------------------------------------

struct AbortPairingRequest
{
    QString m_id;
};

void abortPairing(server::HttpServer& server, server::PairingManager& pairing_manager)
{
    openReqResp(server, "/abortPairing", QHttpServerRequest::Method::Post,
                [&pairing_manager](const AbortPairingRequest& request)
                {
                    const bool result{pairing_manager.abortPairing(request.m_id)};
                    return ResultResponse{.m_result = result};
                });
}

//----------------------------------------------------------------------------------------------------------------------

struct PcStateResponse
{
    enums::PcState m_state;
};

void pcState(server::HttpServer& server, PcControl& pc_control)
{
    secureReqResp(server, "/pcState", QHttpServerRequest::Method::Get,
                  [&pc_control]() { return PcStateResponse{.m_state = pc_control.getPcState()}; });
}

//----------------------------------------------------------------------------------------------------------------------

struct ChangePcStateRequest
{
    Q_GADGET

public:
    enum class ChangePcState
    {
        Restart,
        Shutdown,
        Suspend
    };
    Q_ENUM(ChangePcState)

    ChangePcState m_state;
    uint          m_delay;
};

void changePcState(server::HttpServer& server, PcControl& pc_control)
{
    secureReqResp(server, "/changePcState", QHttpServerRequest::Method::Post,
                  [&pc_control](const ChangePcStateRequest& request)
                      -> std::variant<QHttpServerResponse::StatusCode, ResultResponse>
                  {
                      using enum ChangePcStateRequest::ChangePcState;

                      if (request.m_delay < 1 || 30 < request.m_delay)
                      {
                          qCWarning(lc::buddyMain) << "Delay value is out of range [1;30]:" << request.m_delay;
                          return QHttpServerResponse::StatusCode::BadRequest;
                      }

                      bool result{false};
                      switch (request.m_state)
                      {
                          case Restart:
                              result = pc_control.restartPC(request.m_delay);
                              break;
                          case Shutdown:
                              result = pc_control.shutdownPC(request.m_delay);
                              break;
                          case Suspend:
                              result = pc_control.suspendOrHibernatePC(request.m_delay);
                              break;
                      }
                      return ResultResponse{.m_result = result};
                  });
}

//----------------------------------------------------------------------------------------------------------------------

struct HostInfoResponse
{
    QString m_mac;
    QString m_os;
};

void hostInfo(server::HttpServer& server, const QString& mac_address_override)
{
    secureReqResp(server, "/hostInfo", QHttpServerRequest::Method::Get,
                  [&mac_address_override](const QHttpServerRequest& request)
                      -> std::variant<QHttpServerResponse::StatusCode, HostInfoResponse>
                  {
                      auto mac{mac_address_override.isEmpty() ? os::NetworkInfo::getMacAddress(request.localAddress())
                                                              : mac_address_override};
                      if (mac.isEmpty())
                      {
                          qCWarning(lc::buddyMain) << "could not retrieve MAC address!";
                          return QHttpServerResponse::StatusCode::InternalServerError;
                      }

                      static const QRegularExpression regex{
                          R"(^(?:[[:xdigit:]]{2}([-:]))(?:[[:xdigit:]]{2}\1){4}[[:xdigit:]]{2}$)"};
                      if (!mac.contains(regex))
                      {
                          qCWarning(lc::buddyMain) << "MAC address is invalid:" << mac;
                          return QHttpServerResponse::StatusCode::InternalServerError;
                      }

                      mac.replace('-', ':');

#ifdef Q_OS_WIN
                      const QString os_type{"Windows"};
#elifdef Q_OS_LINUX
                     const QString os_type{"Linux"};
#else
                     const QString os_type{"Other"};
#endif

                      return HostInfoResponse{.m_mac = mac, .m_os = os_type};
                  });
}

//----------------------------------------------------------------------------------------------------------------------

struct SteamUiModeResponse
{
    enums::SteamUiMode m_mode;
};

void steamUiMode(server::HttpServer& server, PcControl& pc_control)
{
    secureReqResp(server, "/steamUiMode", QHttpServerRequest::Method::Get,
                  [&pc_control]()
                  {
                      const auto mode{pc_control.getSteamUiMode()};
                      return SteamUiModeResponse{.m_mode = mode};
                  });
}

//----------------------------------------------------------------------------------------------------------------------

struct NonSteamAppDataRequest
{
    QString m_user_id;
};

struct NonSteamAppDataResponse
{
    struct Entry
    {
        QString m_app_id;
        QString m_app_name;
    };

    std::optional<std::vector<Entry>> m_data;
};

void nonSteamAppData(server::HttpServer& server, PcControl& pc_control)
{
    secureReqResp(server, "/nonSteamAppData", QHttpServerRequest::Method::Get,
                  [&pc_control](const NonSteamAppDataRequest& request)
                      -> std::variant<QHttpServerResponse::StatusCode, NonSteamAppDataResponse>
                  {
                      const auto steam_id{steam::SteamId::fromString(request.m_user_id)};
                      if (!steam_id)
                      {
                          return QHttpServerResponse::StatusCode::BadRequest;
                      }

                      const auto data{pc_control.getNonSteamAppData(*steam_id)};
                      if (!data)
                      {
                          return NonSteamAppDataResponse{.m_data = std::nullopt};
                      }

                      std::vector<NonSteamAppDataResponse::Entry> entries;
                      for (const auto& [app_id, app_name] : *data)
                      {
                          entries.emplace_back(QString::number(app_id.getGameId()), app_name);
                      }

                      return NonSteamAppDataResponse{.m_data = std::move(entries)};
                  });
}

//----------------------------------------------------------------------------------------------------------------------

struct CurrentUserResponse
{
    struct UserData
    {
        std::optional<QString> m_id;
    };

    std::optional<UserData> m_user;
};

void currentUser(server::HttpServer& server, PcControl& pc_control)
{
    secureReqResp(server, "/currentUser", QHttpServerRequest::Method::Get,
                  [&pc_control]()
                  {
                      const auto user_id{pc_control.getCurrentUserId()};
                      if (!user_id)
                      {
                          return CurrentUserResponse{.m_user = std::nullopt};
                      }

                      return CurrentUserResponse{
                          .m_user = CurrentUserResponse::UserData{
                              .m_id = user_id->isNull() ? std::nullopt : std::make_optional(user_id->toSteamId64())}};
                  });
}

//----------------------------------------------------------------------------------------------------------------------

struct LaunchSteamRequest
{
    bool                   m_big_picture_mode;
    std::optional<QString> m_username;
};

void launchSteam(server::HttpServer& server, PcControl& pc_control)
{
    secureReqResp(server, "/launchSteam", QHttpServerRequest::Method::Post,
                  [&pc_control](const LaunchSteamRequest& request)
                  {
                      const bool result{
                          pc_control.launchSteam(request.m_big_picture_mode, request.m_username.value_or(QString{}))};
                      return ResultResponse{.m_result = result};
                  });
}

//----------------------------------------------------------------------------------------------------------------------

struct LaunchSteamAppRequest
{
    QString m_app_id;
};

void launchSteamApp(server::HttpServer& server, PcControl& pc_control)
{
    secureReqResp(server, "/launchSteamApp", QHttpServerRequest::Method::Post,
                  [&pc_control](const LaunchSteamAppRequest& request)
                      -> std::variant<QHttpServerResponse::StatusCode, ResultResponse>
                  {
                      const auto app_id{steam::AppId::fromString(request.m_app_id)};
                      if (!app_id)
                      {
                          return QHttpServerResponse::StatusCode::BadRequest;
                      }

                      const bool result{pc_control.launchSteamApp(*app_id)};
                      return ResultResponse{.m_result = result};
                  });
}

//----------------------------------------------------------------------------------------------------------------------

struct CloseSteamRequest
{
    bool m_keep_stream_alive;
};

void closeSteam(server::HttpServer& server, PcControl& pc_control)
{
    secureReqResp(server, "/closeSteam", QHttpServerRequest::Method::Post,
                  [&pc_control](const CloseSteamRequest& request)
                  {
                      const bool result{pc_control.closeSteam(request.m_keep_stream_alive)};
                      return ResultResponse{.m_result = result};
                  });
}

//----------------------------------------------------------------------------------------------------------------------

void closeSteamBigPictureMode(server::HttpServer& server, PcControl& pc_control)
{
    secureReqResp(server, "/closeSteamBigPictureMode", QHttpServerRequest::Method::Post,
                  [&pc_control]()
                  {
                      const bool result{pc_control.closeSteamBigPictureMode()};
                      return ResultResponse{.m_result = result};
                  });
}

//----------------------------------------------------------------------------------------------------------------------

struct StreamStateResponse
{
    enums::StreamState m_state;
};

void streamState(server::HttpServer& server, PcControl& pc_control)
{
    secureReqResp(server, "/streamState", QHttpServerRequest::Method::Get,
                  [&pc_control]()
                  {
                      const auto state{pc_control.getStreamState()};
                      return StreamStateResponse{.m_state = state};
                  });
}

//----------------------------------------------------------------------------------------------------------------------

struct StreamedAppDataResponse
{
    struct Data
    {
        QString         m_app_id;
        enums::AppState m_app_state;
    };

    std::optional<Data> m_data;
};

void streamedAppData(server::HttpServer& server, PcControl& pc_control)
{
    secureReqResp(server, "/streamedAppData", QHttpServerRequest::Method::Get,
                  [&pc_control]()
                  {
                      const auto data{pc_control.getAppData(std::nullopt)};
                      if (!data)
                      {
                          return StreamedAppDataResponse{.m_data = std::nullopt};
                      }

                      const auto& [app_id, app_state] = *data;
                      return StreamedAppDataResponse{
                          .m_data = StreamedAppDataResponse::Data{.m_app_id    = QString::number(app_id.getId()),
                                                                  .m_app_state = app_state}};
                  });
}

//----------------------------------------------------------------------------------------------------------------------

void clearStreamedAppData(server::HttpServer& server, PcControl& pc_control)
{
    secureReqResp(server, "/clearStreamedAppData", QHttpServerRequest::Method::Post,
                  [&pc_control]()
                  {
                      const auto result{pc_control.clearAppData()};
                      return ResultResponse{.m_result = result};
                  });
}

//----------------------------------------------------------------------------------------------------------------------

void endStream(server::HttpServer& server, PcControl& pc_control)
{
    secureReqResp(server, "/endStream", QHttpServerRequest::Method::Post,
                  [&pc_control]()
                  {
                      const auto result{pc_control.endStream()};
                      return ResultResponse{.m_result = result};
                  });
}

//----------------------------------------------------------------------------------------------------------------------

struct GameStreamAppNamesResponse
{
    std::optional<std::set<QString>> m_app_names;
};

void gameStreamAppNames(server::HttpServer& server, SunshineApps& sunshine_apps)
{
    secureReqResp(server, "/gameStreamAppNames", QHttpServerRequest::Method::Get,
                  [&sunshine_apps]() { return GameStreamAppNamesResponse{.m_app_names = sunshine_apps.load()}; });
}
}  // namespace http_api

void setupRoutes(server::HttpServer& server, server::PairingManager& pairing_manager, PcControl& pc_control,
                 SunshineApps& sunshine_apps, const QString& mac_address_override)
{
    http_api::apiVersion(server);

    http_api::pairingState(server, pairing_manager);
    http_api::pair(server, pairing_manager);
    http_api::abortPairing(server, pairing_manager);

    http_api::pcState(server, pc_control);
    http_api::changePcState(server, pc_control);

    http_api::hostInfo(server, mac_address_override);

    http_api::steamUiMode(server, pc_control);
    http_api::nonSteamAppData(server, pc_control);
    http_api::currentUser(server, pc_control);
    http_api::launchSteam(server, pc_control);
    http_api::launchSteamApp(server, pc_control);
    http_api::closeSteam(server, pc_control);
    http_api::closeSteamBigPictureMode(server, pc_control);

    http_api::streamState(server, pc_control);
    http_api::streamedAppData(server, pc_control);
    http_api::clearStreamedAppData(server, pc_control);
    http_api::endStream(server, pc_control);

    http_api::gameStreamAppNames(server, sunshine_apps);

    server.afterRequest(
        [](const QHttpServerRequest& request, const QHttpServerResponse& resp)
        {
            qCDebug(lc::buddyMain) << Qt::endl
                                   << "Request:" << request << "|" << request.body() << Qt::endl
                                   << "Response:" << resp.statusCode() << "|" << resp.data();
        });
}

// automoc include
#include "routing.moc"
