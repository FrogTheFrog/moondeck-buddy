// header file include
#include "routing.h"

// system/Qt includes
#include <QJsonArray>
#include <QJsonDocument>
#include <limits>

// local includes
#include "os/networkinfo.h"
#include "shared/loggingcategories.h"
#include "utils/jsonvalueconverter.h"

namespace
{
QJsonDocument requestToJson(const QHttpServerRequest& request)
{
    QJsonParseError parser_error;
    QJsonDocument   json_data{QJsonDocument::fromJson(request.body(), &parser_error)};
    if (json_data.isNull())
    {
        qCWarning(lc::buddyMain) << "Failed to decode JSON data! Reason:" << parser_error.errorString()
                                 << "| Body:" << request.body();
        return {};
    }

    if (!request.body().isEmpty() && json_data.isEmpty())
    {
        qCDebug(lc::buddyMain) << "Parsed empty JSON data from:" << request.body();
        return {};
    }

    return json_data;
}

QJsonObject requestToJsonObject(const QHttpServerRequest& request)
{
    const auto json{requestToJson(request)};
    return json.isObject() ? json.object() : QJsonObject{};
}
}  // namespace

namespace routing_internal
{
Q_NAMESPACE

enum class PairingState
{
    Paired,
    Pairing,
    NotPaired
};
Q_ENUM_NS(PairingState)

enum class ChangePcState
{
    Restart,
    Shutdown,
    Suspend
};
Q_ENUM_NS(ChangePcState)

void setupApiVersion(server::HttpServer& server)
{
    server.route("/apiVersion", QHttpServerRequest::Method::Get,
                 [&server]() { return QJsonObject{{"version", server.getApiVersion()}}; });
}

void setupPairing(server::HttpServer& server, server::PairingManager& pairing_manager)
{
    server.route("/pairingState/<arg>", QHttpServerRequest::Method::Get,
                 // NOLINTNEXTLINE(*-identifier-length)
                 [&pairing_manager](const QString& id)
                 {
                     const PairingState state{pairing_manager.isPaired(id)    ? PairingState::Paired
                                              : pairing_manager.isPairing(id) ? PairingState::Pairing
                                                                              : PairingState::NotPaired};

                     return QJsonObject{{"state", lc::qEnumToString(state)}};
                 });

    server.route("/pair", QHttpServerRequest::Method::Post,
                 [&pairing_manager](const QHttpServerRequest& request)
                 {
                     const auto json = requestToJsonObject(request);
                     if (json.isEmpty())
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::BadRequest};
                     }

                     // NOLINTNEXTLINE(*-identifier-length)
                     const auto id{utils::getJsonValue<QString>(json, "id")};
                     const auto hashed_id{utils::getJsonValue<QString>(json, "hashed_id")};
                     if (!id || !hashed_id)
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::BadRequest};
                     }

                     const bool result{pairing_manager.startPairing(*id, *hashed_id)};
                     return QHttpServerResponse{QJsonObject{{"result", result}}};
                 });

    server.route("/abortPairing", QHttpServerRequest::Method::Post,
                 [&pairing_manager](const QHttpServerRequest& request)
                 {
                     const auto json = requestToJsonObject(request);
                     if (json.isEmpty())
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::BadRequest};
                     }

                     // NOLINTNEXTLINE(*-identifier-length)
                     const auto id{utils::getJsonValue<QString>(json, "id")};
                     if (!id)
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::BadRequest};
                     }

                     const bool result{pairing_manager.abortPairing(*id)};
                     return QHttpServerResponse{QJsonObject{{"result", result}}};
                 });
}

void setupPcState(server::HttpServer& server, os::PcControl& pc_control)
{
    server.route("/pcState", QHttpServerRequest::Method::Get,
                 [&server, &pc_control](const QHttpServerRequest& request)
                 {
                     if (!server.isAuthorized(request))
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::Unauthorized};
                     }

                     return QHttpServerResponse{QJsonObject{{"state", lc::qEnumToString(pc_control.getPcState())}}};
                 });

    server.route("/changePcState", QHttpServerRequest::Method::Post,
                 [&server, &pc_control](const QHttpServerRequest& request)
                 {
                     if (!server.isAuthorized(request))
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::Unauthorized};
                     }

                     const auto json = requestToJsonObject(request);
                     if (json.isEmpty())
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::BadRequest};
                     }

                     const auto state{utils::getJsonValue<ChangePcState>(json, "state")};
                     const auto delay{utils::getJsonValue<uint>(json, "delay", 1, 30)};
                     if (!state || !delay)
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::BadRequest};
                     }

                     bool result{false};
                     switch (*state)
                     {
                         case ChangePcState::Restart:
                             result = pc_control.restartPC(*delay);
                             break;
                         case ChangePcState::Shutdown:
                             result = pc_control.shutdownPC(*delay);
                             break;
                         case ChangePcState::Suspend:
                             result = pc_control.suspendOrHibernatePC(*delay);
                             break;
                     }
                     return QHttpServerResponse{QJsonObject{{"result", result}}};
                 });
}

void setupHostInfo(server::HttpServer& server, const QString& mac_address_override)
{
    server.route("/hostInfo", QHttpServerRequest::Method::Get,
                 [&server, &mac_address_override](const QHttpServerRequest& request)
                 {
                     if (!server.isAuthorized(request))
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::Unauthorized};
                     }

                     auto mac{mac_address_override.isEmpty() ? os::NetworkInfo::getMacAddress(request.localAddress())
                                                             : mac_address_override};
                     if (mac.isEmpty())
                     {
                         qCWarning(lc::buddyMain) << "could not retrieve MAC address!";
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::InternalServerError};
                     }
                     else
                     {
                         static const QRegularExpression regex{
                             R"(^(?:[[:xdigit:]]{2}([-:]))(?:[[:xdigit:]]{2}\1){4}[[:xdigit:]]{2}$)"};
                         if (!mac.contains(regex))
                         {
                             qCWarning(lc::buddyMain) << "MAC address is invalid:" << mac;
                             return QHttpServerResponse{QHttpServerResponse::StatusCode::InternalServerError};
                         }

                         mac.replace('-', ':');
                     }

#if defined(Q_OS_WIN)
                     const QString os_type{"Windows"};
#elif defined(Q_OS_LINUX)
                     const QString os_type{"Linux"};
#else
                     const QString os_type{"Other"};
#endif

                     return QHttpServerResponse{QJsonObject{{"mac", mac}, {"os", os_type}}};
                 });
}

void setupSteam(server::HttpServer& server, os::PcControl& pc_control)
{
    server.route("/steamUiMode", QHttpServerRequest::Method::Get,
                 [&server, &pc_control](const QHttpServerRequest& request)
                 {
                     if (!server.isAuthorized(request))
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::Unauthorized};
                     }

                     const auto mode{pc_control.getSteamUiMode()};
                     return QHttpServerResponse{QJsonObject{{"mode", lc::qEnumToString(mode)}}};
                 });

    server.route("/nonSteamAppData", QHttpServerRequest::Method::Get,
                 [&server, &pc_control](const QHttpServerRequest& request)
                 {
                     if (!server.isAuthorized(request))
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::Unauthorized};
                     }

                     const auto json = requestToJsonObject(request);
                     if (json.isEmpty())
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::BadRequest};
                     }

                     const auto user_id_opt{utils::getJsonValue<QString>(json, "user_id")};
                     bool       user_id_ok{false};

                     static_assert(sizeof(qulonglong) == sizeof(std::uint64_t));
                     const std::uint64_t user_id{user_id_opt ? user_id_opt->toULongLong(&user_id_ok) : 0};

                     if (!user_id_ok)
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::BadRequest};
                     }

                     return QHttpServerResponse{
                         [&pc_control, user_id]()
                         {
                             const auto data{pc_control.getNonSteamAppData(user_id)};
                             if (!data)
                             {
                                 return QJsonObject{{"data", QJsonValue::Null}};
                             }

                             QJsonArray json_array;
                             for (const auto& [app_id, app_name] : *data)
                             {
                                 json_array.push_back({{{"app_id", QString::number(app_id)}, {"app_name", app_name}}});
                             }

                             return QJsonObject{{"data", json_array}};
                         }()};
                 });

    server.route("/launchSteam", QHttpServerRequest::Method::Post,
                 [&server, &pc_control](const QHttpServerRequest& request)
                 {
                     if (!server.isAuthorized(request))
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::Unauthorized};
                     }

                     const auto json = requestToJsonObject(request);
                     if (json.isEmpty())
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::BadRequest};
                     }

                     const auto big_picture_mode{utils::getJsonValue<bool>(json, "big_picture_mode")};
                     if (!big_picture_mode)
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::BadRequest};
                     }

                     const bool result{pc_control.launchSteam(*big_picture_mode)};
                     return QHttpServerResponse{QJsonObject{{"result", result}}};
                 });

    server.route("/launchSteamApp", QHttpServerRequest::Method::Post,
                 [&server, &pc_control](const QHttpServerRequest& request)
                 {
                     if (!server.isAuthorized(request))
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::Unauthorized};
                     }

                     const auto json = requestToJsonObject(request);
                     if (json.isEmpty())
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::BadRequest};
                     }

                     const auto app_id_opt{utils::getJsonValue<QString>(json, "app_id")};
                     bool       app_id_ok{false};

                     static_assert(sizeof(qulonglong) == sizeof(std::uint64_t));
                     const std::uint64_t app_id{app_id_opt ? app_id_opt->toULongLong(&app_id_ok) : 0};

                     if (!app_id_ok)
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::BadRequest};
                     }

                     const bool result{pc_control.launchSteamApp(app_id)};
                     return QHttpServerResponse{QJsonObject{{"result", result}}};
                 });

    server.route("/closeSteam", QHttpServerRequest::Method::Post,
                 [&server, &pc_control](const QHttpServerRequest& request)
                 {
                     if (!server.isAuthorized(request))
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::Unauthorized};
                     }

                     const bool result{pc_control.closeSteam()};
                     return QHttpServerResponse{QJsonObject{{"result", result}}};
                 });
}

void setupStream(server::HttpServer& server, os::PcControl& pc_control)
{
    server.route("/streamState", QHttpServerRequest::Method::Get,
                 [&server, &pc_control](const QHttpServerRequest& request)
                 {
                     if (!server.isAuthorized(request))
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::Unauthorized};
                     }

                     const auto state{pc_control.getStreamState()};
                     return QHttpServerResponse{QJsonObject{{"state", lc::qEnumToString(state)}}};
                 });

    server.route("/appData", QHttpServerRequest::Method::Get,
                 [&server, &pc_control](const QHttpServerRequest& request)
                 {
                     if (!server.isAuthorized(request))
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::Unauthorized};
                     }

                     const auto json = requestToJsonObject(request);
                     if (json.isEmpty())
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::BadRequest};
                     }

                     const auto app_id_opt{utils::getJsonValue<QString>(json, "app_id")};
                     bool       app_id_ok{false};

                     static_assert(sizeof(qulonglong) == sizeof(std::uint64_t));
                     const std::uint64_t received_app_id{app_id_opt ? app_id_opt->toULongLong(&app_id_ok) : 0};

                     if (!app_id_ok)
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::BadRequest};
                     }

                     return QHttpServerResponse{
                         [&pc_control, &received_app_id]()
                         {
                             const auto data{pc_control.getAppData(received_app_id)};
                             if (!data)
                             {
                                 return QJsonObject{{"data", QJsonValue::Null}};
                             }

                             const auto& [app_id, app_state] = *data;
                             return QJsonObject{{"data", QJsonObject{{{"app_id", QString::number(app_id)},
                                                                      {"app_state", lc::qEnumToString(app_state)}}}}};
                         }()};
                 });

    server.route("/streamedAppData", QHttpServerRequest::Method::Get,
                 [&server, &pc_control](const QHttpServerRequest& request)
                 {
                     if (!server.isAuthorized(request))
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::Unauthorized};
                     }

                     return QHttpServerResponse{
                         [&pc_control]()
                         {
                             const auto data{pc_control.getAppData(std::nullopt)};
                             if (!data)
                             {
                                 return QJsonObject{{"data", QJsonValue::Null}};
                             }

                             const auto& [app_id, app_state] = *data;
                             return QJsonObject{{"data", QJsonObject{{{"app_id", QString::number(app_id)},
                                                                      {"app_state", lc::qEnumToString(app_state)}}}}};
                         }()};
                 });

    server.route("/clearStreamedAppData", QHttpServerRequest::Method::Post,
                 [&server, &pc_control](const QHttpServerRequest& request)
                 {
                     if (!server.isAuthorized(request))
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::Unauthorized};
                     }

                     const bool result{pc_control.clearAppData()};
                     return QHttpServerResponse{QJsonObject{{"result", result}}};
                 });

    server.route("/endStream", QHttpServerRequest::Method::Post,
                 [&server, &pc_control](const QHttpServerRequest& request)
                 {
                     if (!server.isAuthorized(request))
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::Unauthorized};
                     }

                     const bool result{pc_control.endStream()};
                     return QHttpServerResponse{QJsonObject{{"result", result}}};
                 });
}

void setupRouteLogging(server::HttpServer& server)
{
    server.afterRequest(
        [](const QHttpServerRequest& request, QHttpServerResponse& resp)
        {
            qCDebug(lc::buddyMain) << Qt::endl
                                   << "Request:" << request << "|" << request.body() << Qt::endl
                                   << "Response:" << resp.statusCode() << "|" << resp.data();
        });
}
}  // namespace routing_internal

void setupRoutes(server::HttpServer& server, server::PairingManager& pairing_manager, os::PcControl& pc_control,
                 const QString& mac_address_override)
{
    routing_internal::setupApiVersion(server);
    routing_internal::setupPairing(server, pairing_manager);
    routing_internal::setupPcState(server, pc_control);
    routing_internal::setupHostInfo(server, mac_address_override);
    routing_internal::setupSteam(server, pc_control);
    routing_internal::setupStream(server, pc_control);
    routing_internal::setupRouteLogging(server);
}

// automoc include
#include "routing.moc"
