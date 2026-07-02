// header file include
#include "routing.h"

// local includes
#include "common/statelesssignaldebouncer.h"
#include "os/networkinfo.h"
#include "server/restserver.h"

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

void apiVersion(server::RestServer& server)
{
    server.unauthenticatedHttpRoute("/apiVersion", QHttpServerRequest::Method::Get,
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

void pairingState(server::RestServer& server, server::PairingManager& pairing_manager)
{
    server.unauthenticatedHttpRoute("/pairingState/<arg>", QHttpServerRequest::Method::Get,
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

void pair(server::RestServer& server, server::PairingManager& pairing_manager)
{
    server.unauthenticatedHttpRoute("/pair", QHttpServerRequest::Method::Post,
                                    [&pairing_manager](const PairRequest& request)
                                    {
                                        const bool result{
                                            pairing_manager.startPairing(request.m_id, request.m_hashed_id)};
                                        return ResultResponse{.m_result = result};
                                    });
}

//----------------------------------------------------------------------------------------------------------------------

struct AbortPairingRequest
{
    QString m_id;
};

void abortPairing(server::RestServer& server, server::PairingManager& pairing_manager)
{
    server.unauthenticatedHttpRoute("/abortPairing", QHttpServerRequest::Method::Post,
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

void pcState(server::RestServer& server, PcControl& pc_control)
{
    server.httpRoute("/pcState", QHttpServerRequest::Method::Get,
                     [&pc_control]() { return PcStateResponse{.m_state = pc_control.getPcState()}; });
}

//----------------------------------------------------------------------------------------------------------------------

struct HostStateRequest
{
    std::optional<QHttpServerResponse::StatusCode> validate() const
    {
        constexpr uint min_delay{1};
        constexpr uint max_delay{30};

        if (m_delay < min_delay || max_delay < m_delay)
        {
            qCWarning(lc::buddyMain) << "Delay value is out of range [" << min_delay << ";" << max_delay
                                     << "]:" << m_delay;
            return QHttpServerResponse::StatusCode::BadRequest;
        }

        return std::nullopt;
    }

    uint m_delay;
};

void restartHost(server::RestServer& server, PcControl& pc_control)
{
    server.httpRoute(
        "/restartHost", QHttpServerRequest::Method::Post,
        [&pc_control](const HostStateRequest& request) -> std::variant<QHttpServerResponse::StatusCode, ResultResponse>
        {
            if (const auto err{request.validate()})
            {
                return *err;
            }

            return ResultResponse{.m_result = pc_control.restartPC(request.m_delay)};
        });
}

void shutdownHost(server::RestServer& server, PcControl& pc_control)
{
    server.httpRoute(
        "/shutdownHost", QHttpServerRequest::Method::Post,
        [&pc_control](const HostStateRequest& request) -> std::variant<QHttpServerResponse::StatusCode, ResultResponse>
        {
            if (const auto err{request.validate()})
            {
                return *err;
            }

            return ResultResponse{.m_result = pc_control.shutdownPC(request.m_delay)};
        });
}

void suspendHost(server::RestServer& server, PcControl& pc_control)
{
    server.httpRoute(
        "/suspendHost", QHttpServerRequest::Method::Post,
        [&pc_control](const HostStateRequest& request) -> std::variant<QHttpServerResponse::StatusCode, ResultResponse>
        {
            if (const auto err{request.validate()})
            {
                return *err;
            }

            return ResultResponse{.m_result = pc_control.suspendPC(request.m_delay)};
        });
}

void hibernateHost(server::RestServer& server, PcControl& pc_control)
{
    server.httpRoute(
        "/hibernateHost", QHttpServerRequest::Method::Post,
        [&pc_control](const HostStateRequest& request) -> std::variant<QHttpServerResponse::StatusCode, ResultResponse>
        {
            if (const auto err{request.validate()})
            {
                return *err;
            }

            return ResultResponse{.m_result = pc_control.hibernatePC(request.m_delay)};
        });
}

void abortHostStateChange(server::RestServer& server, PcControl& pc_control)
{
    server.httpRoute("/abortHostStateChange", QHttpServerRequest::Method::Post,
                     [&pc_control]() { return ResultResponse{.m_result = pc_control.abortPcStateChange()}; });
}

//----------------------------------------------------------------------------------------------------------------------

struct HostInfoResponse
{
    QString m_mac;
    QString m_os;
};

void hostInfo(server::RestServer& server, const QString& mac_address_override)
{
    server.httpRoute(
        "/hostInfo", QHttpServerRequest::Method::Get,
        [&mac_address_override](
            const QHttpServerRequest& request) -> std::variant<QHttpServerResponse::StatusCode, HostInfoResponse>
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
    auto operator<=>(const SteamUiModeResponse&) const = default;

    static SteamUiModeResponse makeInstance(const PcControl& pc_control)
    {
        const auto mode{pc_control.getSteamUiMode()};
        return {.m_mode = mode};
    }

    enums::SteamUiMode m_mode;
};

void steamUiMode(server::RestServer& server, PcControl& pc_control)
{
    server.httpRoute("/steamUiMode", QHttpServerRequest::Method::Get,
                     [&pc_control]() { return SteamUiModeResponse::makeInstance(pc_control); });
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

void nonSteamAppData(server::RestServer& server, PcControl& pc_control)
{
    server.httpRoute("/nonSteamAppData", QHttpServerRequest::Method::Get,
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
        auto operator<=>(const UserData&) const = default;

        std::optional<QString> m_id;
    };

    auto operator<=>(const CurrentUserResponse&) const = default;

    static CurrentUserResponse makeInstance(const PcControl& pc_control)
    {
        const auto user_id{pc_control.getCurrentUserId()};
        if (!user_id)
        {
            return {.m_user = std::nullopt};
        }

        return {.m_user =
                    UserData{.m_id = user_id->isNull() ? std::nullopt : std::make_optional(user_id->toSteamId64())}};
    }

    std::optional<UserData> m_user;
};

void currentUser(server::RestServer& server, PcControl& pc_control)
{
    server.httpRoute("/currentUser", QHttpServerRequest::Method::Get,
                     [&pc_control]() { return CurrentUserResponse::makeInstance(pc_control); });
}

//----------------------------------------------------------------------------------------------------------------------

struct LaunchSteamRequest
{
    bool                   m_big_picture_mode;
    std::optional<QString> m_username;
};

void launchSteam(server::RestServer& server, PcControl& pc_control)
{
    server.httpRoute("/launchSteam", QHttpServerRequest::Method::Post,
                     [&pc_control](const LaunchSteamRequest& request)
                     {
                         const bool result{pc_control.launchSteam(request.m_big_picture_mode,
                                                                  request.m_username.value_or(QString{}))};
                         return ResultResponse{.m_result = result};
                     });
}

//----------------------------------------------------------------------------------------------------------------------

struct LaunchSteamAppRequest
{
    QString m_app_id;
};

void launchSteamApp(server::RestServer& server, PcControl& pc_control)
{
    server.httpRoute("/launchSteamApp", QHttpServerRequest::Method::Post,
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

void closeSteam(server::RestServer& server, PcControl& pc_control)
{
    server.httpRoute("/closeSteam", QHttpServerRequest::Method::Post,
                     [&pc_control](const CloseSteamRequest& request)
                     {
                         const bool result{pc_control.closeSteam(request.m_keep_stream_alive)};
                         return ResultResponse{.m_result = result};
                     });
}

//----------------------------------------------------------------------------------------------------------------------

void closeSteamBigPictureMode(server::RestServer& server, PcControl& pc_control)
{
    server.httpRoute("/closeSteamBigPictureMode", QHttpServerRequest::Method::Post,
                     [&pc_control]()
                     {
                         const bool result{pc_control.closeSteamBigPictureMode()};
                         return ResultResponse{.m_result = result};
                     });
}

//----------------------------------------------------------------------------------------------------------------------

struct StreamStateResponse
{
    auto operator<=>(const StreamStateResponse&) const = default;

    static StreamStateResponse makeInstance(const PcControl& pc_control)
    {
        const auto state{pc_control.getStreamState()};
        return {.m_state = state};
    }

    enums::StreamState m_state;
};

void streamState(server::RestServer& server, PcControl& pc_control)
{
    server.httpRoute("/streamState", QHttpServerRequest::Method::Get,
                     [&pc_control]() { return StreamStateResponse::makeInstance(pc_control); });
}

//----------------------------------------------------------------------------------------------------------------------

struct StreamedAppDataResponse
{
    struct Data
    {
        auto operator<=>(const Data&) const = default;

        QString         m_app_id;
        enums::AppState m_app_state;
    };

    auto operator<=>(const StreamedAppDataResponse&) const = default;

    static StreamedAppDataResponse makeInstance(const PcControl& pc_control)
    {
        const auto data{pc_control.getAppData(std::nullopt)};
        if (!data)
        {
            return {.m_data = std::nullopt};
        }

        const auto& [app_id, app_state] = *data;
        return {.m_data = Data{.m_app_id = QString::number(app_id.getId()), .m_app_state = app_state}};
    }

    std::optional<Data> m_data;
};

void streamedAppData(server::RestServer& server, PcControl& pc_control)
{
    server.httpRoute("/streamedAppData", QHttpServerRequest::Method::Get,
                     [&pc_control]() { return StreamedAppDataResponse::makeInstance(pc_control); });
}

//----------------------------------------------------------------------------------------------------------------------

void clearStreamedAppData(server::RestServer& server, PcControl& pc_control)
{
    server.httpRoute("/clearStreamedAppData", QHttpServerRequest::Method::Post,
                     [&pc_control]()
                     {
                         const auto result{pc_control.clearAppData()};
                         return ResultResponse{.m_result = result};
                     });
}

//----------------------------------------------------------------------------------------------------------------------

void endStream(server::RestServer& server, PcControl& pc_control)
{
    server.httpRoute("/endStream", QHttpServerRequest::Method::Post,
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

void gameStreamAppNames(server::RestServer& server, SunshineApps& sunshine_apps)
{
    server.httpRoute("/gameStreamAppNames", QHttpServerRequest::Method::Get,
                     [&sunshine_apps]() { return GameStreamAppNamesResponse{.m_app_names = sunshine_apps.load()}; });
}
}  // namespace http_api

namespace websocket_api
{
using ResultResponse = http_api::ResultResponse;

//----------------------------------------------------------------------------------------------------------------------

using StreamedAppData  = http_api::StreamedAppDataResponse;
using SteamUiMode      = http_api::SteamUiModeResponse;
using CurrentUser      = http_api::CurrentUserResponse;
using StreamState      = http_api::StreamStateResponse;
using NotificationType = std::variant<StreamedAppData, SteamUiMode, CurrentUser, StreamState>;

class NotificationTracker : public QObject
{
    Q_OBJECT

public:
    enum class NotificationTopic
    {
        StreamedAppData,
        SteamUiMode,
        CurrentUser,
        StreamState
    };
    Q_ENUM(NotificationTopic)

    explicit NotificationTracker(server::WebSocket& socket, PcControl& pc_control,
                                 std::vector<NotificationTopic> topics)
        : m_socket{socket}
        , m_pc_control{pc_control}
        , m_topics{std::move(topics)}
    {
        connect(&m_debouncer, &common::StatelessSignalDebouncer::signalOutput, this,
                &NotificationTracker::slotSyncData);

        for (const auto& topic : m_topics)
        {
            switch (topic)
            {
                case NotificationTopic::StreamedAppData:
                    connect(&m_pc_control, &PcControl::signalTrackedAppDataChanged, &m_debouncer,
                            &common::StatelessSignalDebouncer::signalInput);
                    break;
                case NotificationTopic::SteamUiMode:
                    connect(&m_pc_control, &PcControl::signalSteamUiModeChanged, &m_debouncer,
                            &common::StatelessSignalDebouncer::signalInput);
                    break;
                case NotificationTopic::CurrentUser:
                    connect(&m_pc_control, &PcControl::signalSteamCurrentUserChanged, &m_debouncer,
                            &common::StatelessSignalDebouncer::signalInput);
                    break;
                case NotificationTopic::StreamState:
                    connect(&m_pc_control, &PcControl::signalStreamStateChanged, &m_debouncer,
                            &common::StatelessSignalDebouncer::signalInput);
                    break;
            }
        }
    }
    ~NotificationTracker() override = default;

public slots:
    void slotSyncData()
    {
        std::vector<NotificationType> new_data;
        new_data.reserve(m_topics.size());

        for (const auto& topic : m_topics)
        {
            switch (topic)
            {
                case NotificationTopic::StreamedAppData:
                    new_data.emplace_back(StreamedAppData::makeInstance(m_pc_control));
                    break;
                case NotificationTopic::SteamUiMode:
                    new_data.emplace_back(SteamUiMode::makeInstance(m_pc_control));
                    break;
                case NotificationTopic::CurrentUser:
                    new_data.emplace_back(CurrentUser::makeInstance(m_pc_control));
                    break;
                case NotificationTopic::StreamState:
                    new_data.emplace_back(StreamState::makeInstance(m_pc_control));
                    break;
            }
        }

        if (new_data != m_last_sent_data)
        {
            m_socket.sendJson(new_data);
            m_last_sent_data = std::move(new_data);
        }
    }

private:
    server::WebSocket&             m_socket;
    PcControl&                     m_pc_control;
    std::vector<NotificationTopic> m_topics;

    common::StatelessSignalDebouncer m_debouncer;
    std::vector<NotificationType>    m_last_sent_data;
};

void notifyOnChanges(server::RestServer& server, PcControl& pc_control)
{
    server.webSocketRoute(
        "/notifyOnChanges",
        [&pc_control](server::WebSocket&                                         socket,
                      const std::vector<NotificationTracker::NotificationTopic>& topics) -> ResultResponse
        {
            if (topics.empty())
            {
                qCWarning(lc::buddyMain).noquote() << socket.getIdString() << "no notification topics provided!";
                return {false};
            }

            // The vector item order must be preserved, but using set to at least verify uniqueness
            {
                if (const std::set<NotificationTracker::NotificationTopic> unique_notifications{std::begin(topics),
                                                                                                std::end(topics)};
                    unique_notifications.size() != topics.size())
                {
                    qCWarning(lc::buddyMain).noquote()
                        << socket.getIdString() << "duplicates found in notification topics!";
                    return {false};
                }
            }

            const auto& tracker{socket.createOrOverrideStoredData<NotificationTracker>(socket, pc_control, topics)};
            QTimer::singleShot(0, &tracker, &NotificationTracker::slotSyncData);

            return {true};
        });
}
}  // namespace websocket_api

void setupRoutes(server::RestServer& server, server::PairingManager& pairing_manager, PcControl& pc_control,
                 SunshineApps& sunshine_apps, const QString& mac_address_override)
{
    http_api::apiVersion(server);

    http_api::pairingState(server, pairing_manager);
    http_api::pair(server, pairing_manager);
    http_api::abortPairing(server, pairing_manager);

    http_api::pcState(server, pc_control);
    http_api::restartHost(server, pc_control);
    http_api::shutdownHost(server, pc_control);
    http_api::suspendHost(server, pc_control);
    http_api::hibernateHost(server, pc_control);
    http_api::abortHostStateChange(server, pc_control);

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

    websocket_api::notifyOnChanges(server, pc_control);

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
