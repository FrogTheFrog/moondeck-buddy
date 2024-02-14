// header file include
#include "routing.h"

// system/Qt includes
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtNetwork/QNetworkInterface>
#include <limits>

// local includes
#include "shared/loggingcategories.h"
#include "utils/jsonvalueconverter.h"

//---------------------------------------------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------------------------------------------

QJsonObject requestToJsonObject(const QHttpServerRequest& request)
{
    const auto json{requestToJson(request)};
    return json.isObject() ? json.object() : QJsonObject{};
}

//---------------------------------------------------------------------------------------------------------------------

QString getMacAddress(const QHostAddress& address)
{
    static const QSet<QNetworkInterface::InterfaceType> allowed_types{QNetworkInterface::Ethernet,
                                                                      QNetworkInterface::Wifi};

    for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces())
    {
        if (allowed_types.contains(iface.type()) && (iface.flags() & QNetworkInterface::IsRunning))
        {
            for (const QHostAddress& address_entry : iface.allAddresses())
            {
                if (address_entry.isEqual(address))
                {
                    return iface.hardwareAddress();
                }
            }
        }
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------

const int MAX_GRACE_PERIOD_S{30};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace routing_internal
{
Q_NAMESPACE

//---------------------------------------------------------------------------------------------------------------------

enum class PairingState
{
    Paired,
    Pairing,
    NotPaired
};
Q_ENUM_NS(PairingState)

//---------------------------------------------------------------------------------------------------------------------

enum class ChangePcState
{
    Restart,
    Shutdown,
    Suspend
};
Q_ENUM_NS(ChangePcState)

//---------------------------------------------------------------------------------------------------------------------

void setupApiVersion(server::HttpServer& server)
{
    server.route("/apiVersion", QHttpServerRequest::Method::Get,
                 [&server]() {
                     return QJsonObject{{"version", server.getApiVersion()}};
                 });
}

//---------------------------------------------------------------------------------------------------------------------

void setupPairing(server::HttpServer& server, server::PairingManager& pairing_manager)
{
    server.route("/pairingState/<arg>", QHttpServerRequest::Method::Get,
                 // NOLINTNEXTLINE(*-identifier-length)
                 [&pairing_manager](const QString& id)
                 {
                     const PairingState state{pairing_manager.isPaired(id)    ? PairingState::Paired
                                              : pairing_manager.isPairing(id) ? PairingState::Pairing
                                                                              : PairingState::NotPaired};

                     return QJsonObject{{"state", QVariant::fromValue(state).toString()}};
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

//---------------------------------------------------------------------------------------------------------------------

void setupPcState(server::HttpServer& server, os::PcControl& pc_control, bool prefer_hibernation,
                  bool close_steam_before_sleep)
{
    server.route(
        "/pcState", QHttpServerRequest::Method::Get,
        [&server, &pc_control](const QHttpServerRequest& request)
        {
            if (!server.isAuthorized(request))
            {
                return QHttpServerResponse{QHttpServerResponse::StatusCode::Unauthorized};
            }

            return QHttpServerResponse{QJsonObject{{"state", QVariant::fromValue(pc_control.getPcState()).toString()}}};
        });

    server.route("/changePcState", QHttpServerRequest::Method::Post,
                 [&server, &pc_control, prefer_hibernation, close_steam_before_sleep](const QHttpServerRequest& request)
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
                     const auto grace_period{utils::getJsonValue<uint>(json, "grace_period", 0, MAX_GRACE_PERIOD_S)};
                     if (!state || !grace_period)
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::BadRequest};
                     }

                     bool result{false};
                     switch (*state)
                     {
                         case ChangePcState::Restart:
                             result = pc_control.restartPC(*grace_period);
                             break;
                         case ChangePcState::Shutdown:
                             result = pc_control.shutdownPC(*grace_period);
                             break;
                         case ChangePcState::Suspend:
                             result =
                                 (prefer_hibernation && pc_control.hibernatePC(*grace_period, close_steam_before_sleep))
                                 || pc_control.suspendPC(*grace_period, close_steam_before_sleep);
                             break;
                     }
                     return QHttpServerResponse{QJsonObject{{"result", result}}};
                 });
}

//---------------------------------------------------------------------------------------------------------------------

void setupHostInfo(server::HttpServer& server, os::PcControl& pc_control)
{
    server.route("/hostInfo", QHttpServerRequest::Method::Get,
                 [&server, &pc_control](const QHttpServerRequest& request)
                 {
                     if (!server.isAuthorized(request))
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::Unauthorized};
                     }

                     auto optional_int = [](std::optional<uint> value)
                     { return value ? QJsonValue{static_cast<int>(*value)} : QJsonValue{QJsonValue::Null}; };

                     return QHttpServerResponse{
                         QJsonObject{{"steamIsRunning", pc_control.isSteamRunning()},
                                     {"steamRunningAppId", static_cast<int>(pc_control.getRunningApp())},
                                     {"steamTrackedUpdatingAppId", optional_int(pc_control.getTrackedUpdatingApp())},
                                     {"streamState", QVariant::fromValue(pc_control.getStreamState()).toString()}}};
                 });
}

//---------------------------------------------------------------------------------------------------------------------

void setupHostPcInfo(server::HttpServer& server, const QString& mac_address_override)
{
    server.route("/hostPcInfo", QHttpServerRequest::Method::Get,
                 [&server, &mac_address_override](const QHttpServerRequest& request)
                 {
                     if (!server.isAuthorized(request))
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::Unauthorized};
                     }

                     auto mac{mac_address_override.isEmpty() ? getMacAddress(request.localAddress())
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

//---------------------------------------------------------------------------------------------------------------------

void setupSteamControl(server::HttpServer& server, os::PcControl& pc_control, bool force_big_picture)
{
    server.route("/launchSteamApp", QHttpServerRequest::Method::Post,
                 [&server, &pc_control, force_big_picture](const QHttpServerRequest& request)
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

                     const auto app_id{utils::getJsonValue<uint>(json, "app_id")};
                     if (!app_id)
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::BadRequest};
                     }

                     const bool result{pc_control.launchSteamApp(*app_id, force_big_picture)};
                     return QHttpServerResponse{QJsonObject{{"result", result}}};
                 });

    server.route("/closeSteam", QHttpServerRequest::Method::Post,
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

                     const auto grace_period{
                         utils::getNullableJsonValue<uint>(json, "grace_period", 0, MAX_GRACE_PERIOD_S)};
                     if (!grace_period)
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::BadRequest};
                     }

                     const bool result{pc_control.closeSteam(*grace_period)};
                     return QHttpServerResponse{QJsonObject{{"result", result}}};
                 });
}

//---------------------------------------------------------------------------------------------------------------------

void setupResolution(server::HttpServer& server, os::PcControl& pc_control)
{
    server.route("/changeResolution", QHttpServerRequest::Method::Post,
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

                     const auto width{utils::getJsonValue<uint>(json, "width")};
                     const auto height{utils::getJsonValue<uint>(json, "height")};
                     if (!width || !height)
                     {
                         return QHttpServerResponse{QHttpServerResponse::StatusCode::BadRequest};
                     }

                     const bool result{pc_control.changeResolution(*width, *height)};
                     return QHttpServerResponse{QJsonObject{{"result", result}}};
                 });
}

//---------------------------------------------------------------------------------------------------------------------

void setupStreamControl(server::HttpServer& server, os::PcControl& pc_control)
{
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

//---------------------------------------------------------------------------------------------------------------------

void setupGamestreamApps(server::HttpServer& server, os::SunshineApps& sunshine_apps)
{
    server.route(
        "/gamestreamAppNames", QHttpServerRequest::Method::Get,
        [&server, &sunshine_apps](const QHttpServerRequest& request)
        {
            if (!server.isAuthorized(request))
            {
                return QHttpServerResponse{QHttpServerResponse::StatusCode::Unauthorized};
            }

            const auto app_names{sunshine_apps.load()};
            if (!app_names)
            {
                return QHttpServerResponse{QJsonObject{{"appNames", QJsonValue()}}};
            }

            return QHttpServerResponse{QJsonObject{
                {"appNames", QJsonArray::fromStringList(QStringList{std::begin(*app_names), std::end(*app_names)})}}};
        });
}

//---------------------------------------------------------------------------------------------------------------------

void setupRouteLogging(server::HttpServer& server)
{
    server.afterRequest(
        [](const QHttpServerRequest& request, QHttpServerResponse&& resp)
        {
            qCDebug(lc::buddyMain) << Qt::endl
                                   << "Request:" << request << "|" << request.body() << Qt::endl
                                   << "Response:" << resp.statusCode() << "|" << resp.data();
            return std::move(resp);
        });
}
}  // namespace routing_internal

//---------------------------------------------------------------------------------------------------------------------

void setupRoutes(server::HttpServer& server, server::PairingManager& pairing_manager, os::PcControl& pc_control,
                 os::SunshineApps& sunshine_apps, bool prefer_hibernation, bool force_big_picture,
                 bool close_steam_before_sleep, const QString& mac_address_override)
{
    routing_internal::setupApiVersion(server);
    routing_internal::setupPairing(server, pairing_manager);
    routing_internal::setupPcState(server, pc_control, prefer_hibernation, close_steam_before_sleep);
    routing_internal::setupHostInfo(server, pc_control);
    routing_internal::setupHostPcInfo(server, mac_address_override);
    routing_internal::setupSteamControl(server, pc_control, force_big_picture);
    routing_internal::setupResolution(server, pc_control);
    routing_internal::setupStreamControl(server, pc_control);
    routing_internal::setupGamestreamApps(server, sunshine_apps);
    routing_internal::setupRouteLogging(server);
}

//---------------------------------------------------------------------------------------------------------------------

// automoc include
#include "routing.moc"
