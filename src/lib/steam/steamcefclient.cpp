// header file include
#include "steam/steamcefclient.h"

// system/Qt includes
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QWebSocket>

// local includes
#include "common/loggingcategories.h"
#include "json/json.h"

namespace
{
constexpr int REQUEST_TIMEOUT_MS{5000};
constexpr int EVALUATE_MESSAGE_ID{1};
}  // namespace

// The message types have to live in a named namespace: glaze's reflection does not work on types
// without linkage.
namespace steam::cef
{
// A page listed by the debugger's /json endpoint. The Steam client's own API lives in the page
// called "SharedJSContext".
struct DebuggerTarget
{
    QString m_title;
    QString m_web_socket_debugger_url;
};

struct EvaluateParams
{
    QString m_expression;
    bool    m_await_promise{true};
    bool    m_return_by_value{true};
};

struct EvaluateRequest
{
    int            m_id{EVALUATE_MESSAGE_ID};
    QString        m_method{"Runtime.evaluate"};
    EvaluateParams m_params;
};

struct EvaluateException
{
    std::optional<QString> m_description;
};

struct EvaluateExceptionDetails
{
    std::optional<QString>           m_text;
    std::optional<EvaluateException> m_exception;
};

struct EvaluateResult
{
    std::optional<EvaluateExceptionDetails> m_exception_details;
};

struct EvaluateError
{
    std::optional<QString> m_message;
};

struct EvaluateResponse
{
    std::optional<int>            m_id;
    std::optional<EvaluateResult> m_result;
    std::optional<EvaluateError>  m_error;
};
}  // namespace steam::cef

// The debugger speaks camelCase, so the keys are spelled out instead of derived from the members.
namespace glz
{
template<>
struct meta<steam::cef::DebuggerTarget>
{
    using T                     = steam::cef::DebuggerTarget;
    static constexpr auto value = object("title", &T::m_title, "webSocketDebuggerUrl", &T::m_web_socket_debugger_url);
};

template<>
struct meta<steam::cef::EvaluateParams>
{
    using T                     = steam::cef::EvaluateParams;
    static constexpr auto value = object("expression", &T::m_expression, "awaitPromise", &T::m_await_promise,
                                         "returnByValue", &T::m_return_by_value);
};

template<>
struct meta<steam::cef::EvaluateExceptionDetails>
{
    using T                     = steam::cef::EvaluateExceptionDetails;
    static constexpr auto value = object("text", &T::m_text, "exception", &T::m_exception);
};

template<>
struct meta<steam::cef::EvaluateResult>
{
    using T                     = steam::cef::EvaluateResult;
    static constexpr auto value = object("exceptionDetails", &T::m_exception_details);
};
}  // namespace glz

namespace steam
{
SteamCefClient::SteamCefClient(QObject* parent)
    : QObject{parent}
    , m_network_manager{std::make_unique<QNetworkAccessManager>()}
    , m_socket{std::make_unique<QWebSocket>()}
{
    m_timeout_timer.setSingleShot(true);
    m_timeout_timer.setInterval(REQUEST_TIMEOUT_MS);

    connect(&m_timeout_timer, &QTimer::timeout, this, &SteamCefClient::slotTimeout);
    connect(m_socket.get(), &QWebSocket::connected, this, &SteamCefClient::slotSocketConnected);
    connect(m_socket.get(), &QWebSocket::textMessageReceived, this, &SteamCefClient::slotSocketTextMessageReceived);
    connect(m_socket.get(), &QWebSocket::errorOccurred, this, &SteamCefClient::slotSocketError);
}

// For forward declarations
SteamCefClient::~SteamCefClient() = default;

void SteamCefClient::runGame(const quint16 port, const QString& game_id, FinishedCallback callback)
{
    finish(false, "Superseded by a newer request");

    m_game_id  = game_id;
    m_callback = std::move(callback);

    const QUrl url{QStringLiteral("http://127.0.0.1:%1/json").arg(port)};
    qCInfo(lc::steam) << "Asking the Steam client via" << url.toString() << "to run game id" << m_game_id;

    m_reply = m_network_manager->get(QNetworkRequest{url});
    connect(m_reply, &QNetworkReply::finished, this, [this, reply = m_reply]() { handleTargetList(reply); });
    m_timeout_timer.start();
}

void SteamCefClient::slotSocketConnected()
{
    if (!m_callback)
    {
        return;
    }

    // The id is passed as a string on purpose: shortcut game ids do not fit into a JS number.
    const cef::EvaluateRequest request{
        .m_params = cef::EvaluateParams{
            .m_expression = QStringLiteral(R"(SteamClient.Apps.RunGame("%1", "", -1, 100))").arg(m_game_id)}};

    const auto message{json::toJson(request)};
    if (!message)
    {
        finish(false, "Could not encode the evaluate request: " + message.error());
        return;
    }

    m_socket->sendTextMessage(*message);
}

void SteamCefClient::slotSocketTextMessageReceived(const QString& message)
{
    if (!m_callback)
    {
        return;
    }

    const auto response{
        json::fromJson<cef::EvaluateResponse, {.m_allow_unknown_keys = true, .m_allow_missing_keys = true}>(message)};
    if (!response)
    {
        finish(false, "Could not parse the debugger response: " + response.error());
        return;
    }

    const cef::EvaluateResponse& parsed{response.value()};
    if (parsed.m_id != EVALUATE_MESSAGE_ID)
    {
        // Not an answer to our request (events and such), keep waiting.
        return;
    }

    if (parsed.m_error.has_value())
    {
        const cef::EvaluateError& error{parsed.m_error.value()};
        finish(false,
               "The debugger rejected the request: " + error.m_message.value_or(QStringLiteral("unknown error")));
        return;
    }

    if (parsed.m_result.has_value())
    {
        const cef::EvaluateResult& result{parsed.m_result.value()};
        if (result.m_exception_details.has_value())
        {
            const cef::EvaluateExceptionDetails& details{result.m_exception_details.value()};
            QString                              description{details.m_text.value_or(QStringLiteral("unknown error"))};
            if (details.m_exception.has_value())
            {
                const cef::EvaluateException& exception{details.m_exception.value()};
                if (exception.m_description.has_value())
                {
                    description = exception.m_description.value();
                }
            }

            finish(false, "Steam threw an exception for the RunGame call: " + description);
            return;
        }
    }

    finish(true, {});
}

void SteamCefClient::slotSocketError()
{
    if (!m_callback)
    {
        return;
    }

    finish(false, "Connection to the debugger failed: " + m_socket->errorString());
}

void SteamCefClient::slotTimeout()
{
    finish(false, "Timed out waiting for the Steam client");
}

void SteamCefClient::handleTargetList(QNetworkReply* reply)
{
    reply->deleteLater();
    if (reply != m_reply)
    {
        // Belongs to a request that was already dropped.
        return;
    }
    m_reply = nullptr;

    if (reply->error() != QNetworkReply::NoError)
    {
        finish(false, "Could not reach the debugger: " + reply->errorString());
        return;
    }

    const auto targets{
        json::fromJson<std::vector<cef::DebuggerTarget>, {.m_allow_unknown_keys = true, .m_allow_missing_keys = true}>(
            QString::fromUtf8(reply->readAll()))};
    if (!targets)
    {
        finish(false, "Could not parse the debugger target list: " + targets.error());
        return;
    }

    const auto target{std::ranges::find_if(*targets, [](const auto& item)
                                           { return item.m_title == QStringLiteral("SharedJSContext"); })};
    if (target == targets->end())
    {
        finish(false, "The debugger does not list the SharedJSContext page");
        return;
    }

    m_socket->open(QUrl{target->m_web_socket_debugger_url});
}

void SteamCefClient::finish(const bool success, const QString& reason)
{
    // Taking the callback first makes the cleanup below safe against re-entry (aborting the
    // socket may emit an error signal, which lands here again and does nothing).
    const auto callback{std::exchange(m_callback, nullptr)};
    if (!callback)
    {
        return;
    }

    m_timeout_timer.stop();

    if (m_reply != nullptr)
    {
        auto* reply{std::exchange(m_reply, nullptr)};
        reply->abort();
        reply->deleteLater();
    }

    if (m_socket->state() != QAbstractSocket::UnconnectedState)
    {
        m_socket->abort();
    }

    if (success)
    {
        qCInfo(lc::steam) << "The Steam client accepted the RunGame call for game id" << m_game_id;
    }
    else
    {
        qCWarning(lc::steam) << "Could not run game id" << m_game_id << "via the Steam client:" << reason;
    }

    callback(success);
}
}  // namespace steam
