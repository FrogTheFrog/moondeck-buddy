#pragma once

// system/Qt includes
#include <QTimer>
#include <QUuid>
#include <QtNetwork/QAbstractSocket>

// local includes
#include "cobsjsonprotocol.h"

//---------------------------------------------------------------------------------------------------------------------

namespace server
{
class JsonSocket : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(JsonSocket)

public:
    explicit JsonSocket(QUuid socket_id, QAbstractSocket* socket);
    ~JsonSocket() override;

    const QUuid& getSocketId() const;
    bool         isOpen() const;

    void setResponseTimeout(uint msecs);
    void write(const QJsonDocument& data);
    void close();

signals:
    void signalDisconnected(const QUuid& socket_id);
    void signalDataReceived(const QUuid& socket_id, const QJsonDocument& data);

private:
    QUuid            m_socket_id;
    QTimer           m_response_timeout;
    QAbstractSocket* m_socket;
    CobsJsonProtocol m_protocol{512};
};
}  // namespace server
