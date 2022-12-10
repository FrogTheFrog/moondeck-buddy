// header file include
#include "cobsjsonprotocol.h"

// local includes
#include "cobs.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const qsizetype END_OF_ARRAY{-1};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace server
{
CobsJsonProtocol::CobsJsonProtocol(std::optional<qsizetype> max_buffer_size)
    : m_max_buffer_size{max_buffer_size}
{
    connect(this, &CobsJsonProtocol::signalSendJsonData, this, &CobsJsonProtocol::slotHandleSendingJsonData);
    connect(this, &CobsJsonProtocol::signalRawDataReceived, this, &CobsJsonProtocol::slotHandleReceivedRawData);
}

//---------------------------------------------------------------------------------------------------------------------

void CobsJsonProtocol::slotHandleSendingJsonData(const QJsonDocument& data)
{
    if (!data.isEmpty())
    {
        const QByteArray data_to_send{cobs::encode(data.toJson(QJsonDocument::Compact)) + cobs::SEPARATOR};
        emit             signalSendRawData(data_to_send);
    }
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void CobsJsonProtocol::slotHandleReceivedRawData(const QByteArray& data)
{
    const auto next = [&data](const qsizetype index)
    {
        const auto next_index = index + 1;
        return next_index < data.size() ? next_index : END_OF_ARRAY;
    };
    const auto parse_data = [&](const qsizetype from)
    {
        const auto separator_index = data.indexOf(cobs::SEPARATOR, from);
        if (separator_index == -1)
        {
            if (m_discarded_data)
            {
                qWarning("Discarding received data and waiting for a new data array!");
                return END_OF_ARRAY;
            }

            m_buffer += data.sliced(from);
            return END_OF_ARRAY;
        }

        if (m_discarded_data)
        {
            m_discarded_data = false;
            qWarning("Discarding received data and resetting state!");
            return next(separator_index);
        }

        const auto new_data = data.sliced(from, separator_index - from);
        const auto buffer_to_decode{m_buffer.isEmpty() ? new_data : (m_buffer + new_data)};

        if (!buffer_to_decode.isEmpty())
        {
            const auto decoded_data = cobs::decode(buffer_to_decode);
            if (decoded_data)
            {
                QJsonParseError     parser_error;
                const QJsonDocument json_data{QJsonDocument::fromJson(*decoded_data, &parser_error)};
                if (json_data.isNull())
                {
                    qWarning("Failed to decode JSON data! Reason: %s. Parsed data: %s",
                             qUtf8Printable(parser_error.errorString()),
                             qUtf8Printable(decoded_data->toHex(' ').toUpper()));
                }
                else if (json_data.isEmpty())
                {
                    qInfo("Parsed empty JSON data from: %s", qUtf8Printable(decoded_data->toHex(' ').toUpper()));
                }
                else
                {
                    emit signalJsonDataReceived(json_data);
                }
            }
            else
            {
                qWarning("Failed to decode: %s", qUtf8Printable(buffer_to_decode.toHex(' ').toUpper()));
            }
        }
        else
        {
            qDebug("Skipping decoding of empty buffer!");
        }

        m_buffer.clear();
        return next(separator_index);
    };

    qsizetype index = parse_data(0);
    while (index != END_OF_ARRAY)
    {
        index = parse_data(index);
    }

    if (m_max_buffer_size && m_buffer.size() > *m_max_buffer_size)
    {
        qWarning("Flushing JSON buffer!");
        m_discarded_data = true;
        m_buffer.clear();
    }
}
}  // namespace server
