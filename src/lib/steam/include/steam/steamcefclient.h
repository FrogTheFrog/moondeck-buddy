#pragma once

// system/Qt includes
#include <QObject>
#include <QTimer>
#include <functional>
#include <memory>

// forward declarations
class QNetworkAccessManager;
class QNetworkReply;
class QWebSocket;

namespace steam
{
//! Talks to the running Steam client through its CEF remote debugging port and asks it to run
//! apps directly (the same SteamClient.Apps.RunGame call MoonDeck itself uses on the Deck), so
//! that no second Steam process has to be spawned just to hand over a steam:// URL.
class SteamCefClient final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SteamCefClient)

public:
    using FinishedCallback = std::function<void(bool success)>;

    explicit SteamCefClient(QObject* parent = nullptr);
    ~SteamCefClient() override;

    //! Asks Steam to run the game. The callback is invoked exactly once, either when Steam has
    //! accepted the call or when anything along the way fails. A request that is still in
    //! progress is dropped first (its callback receives false).
    void runGame(quint16 port, const QString& game_id, FinishedCallback callback);

private slots:
    void slotSocketConnected();
    void slotSocketTextMessageReceived(const QString& message);
    void slotSocketError();
    void slotTimeout();

private:
    void handleTargetList(QNetworkReply* reply);
    void finish(bool success, const QString& reason);

    std::unique_ptr<QNetworkAccessManager> m_network_manager;
    std::unique_ptr<QWebSocket>            m_socket;
    QTimer                                 m_timeout_timer;
    QNetworkReply*                         m_reply{nullptr};
    QString                                m_game_id;
    FinishedCallback                       m_callback;
};
}  // namespace steam
