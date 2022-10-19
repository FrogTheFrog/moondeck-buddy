#pragma once

// system/Qt includes
#include <QByteArray>
#include <QJsonDocument>
#include <QObject>

//---------------------------------------------------------------------------------------------------------------------

namespace server
{
//! Json Protocol based on COBS encoding/decoding
class CobsJsonProtocol : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(CobsJsonProtocol)

public:
    explicit CobsJsonProtocol(std::optional<qsizetype> max_buffer_size);
    ~CobsJsonProtocol() override = default;

signals:
    void signalSendRawData(const QByteArray& data);
    void signalSendJsonData(const QJsonDocument& data);

    void signalRawDataReceived(const QByteArray& data);
    void signalJsonDataReceived(const QJsonDocument& data);

private slots:
    void slotHandleSendingJsonData(const QJsonDocument& data);
    void slotHandleReceivedRawData(const QByteArray& data);

// NOLINTNEXTLINE(readability-redundant-access-specifiers)
private:
    QByteArray               m_buffer;
    std::optional<qsizetype> m_max_buffer_size;
    bool                     m_discarded_data{false};
};
}  // namespace server
