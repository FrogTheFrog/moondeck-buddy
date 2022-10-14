// ----------------------------------------------------------------------------
// Copyright (c) 2010 Craig McQueen
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ----------------------------------------------------------------------------

// header file include
#include "cobs.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
qsizetype getMaxPosibleSizeAfterEncoding(qsizetype input_size)
{
    // NOLINTNEXTLINE(*-magic-numbers)
    return input_size + ((input_size + 253U) / 254U);
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace server::cobs
{
QByteArray encode(const QByteArray& data)
{
    const auto data_size{data.size()};
    if (data_size == 0)
    {
        return {};
    }

    QByteArray output;
    output.resize(getMaxPosibleSizeAfterEncoding(data_size));

    uint8_t   length_code{1};
    qsizetype code_index{0};
    qsizetype output_index{1};
    for (qsizetype data_index = 0;;)
    {
        const uint8_t data_byte{static_cast<uint8_t>(data[data_index++])};
        if (data_byte == 0)
        {
            output[code_index] = static_cast<int8_t>(length_code);
            code_index         = output_index++;
            length_code        = 1;

            if (data_index == data_size)
            {
                break;
            }
        }
        else
        {
            output[output_index++] = static_cast<int8_t>(data_byte);
            length_code++;

            if (data_index == data_size)
            {
                break;
            }
            
            // NOLINTNEXTLINE(*-magic-numbers)
            if (length_code == 0xFF)
            {
                output[code_index] = static_cast<int8_t>(length_code);
                code_index         = output_index++;
                length_code        = 1;
            }
        }
    }

    output[code_index] = static_cast<int8_t>(length_code);
    output.resize(output_index);

    return output;
}

//---------------------------------------------------------------------------------------------------------------------

std::optional<QByteArray> decode(const QByteArray& data)
{
    const auto data_size{data.size()};
    if (data_size == 0)
    {
        return QByteArray{};
    }

    QByteArray output;
    output.resize(data_size);

    qsizetype output_index{0};
    for (qsizetype data_index = 0;;)
    {
        uint8_t length_code{static_cast<uint8_t>(data[data_index++])};
        if (length_code == 0)
        {
            qWarning("0x00 byte in the data array! Bailing...");
            return std::nullopt;
        }
        length_code--;

        const uint8_t remaining_bytes = data_size - data_index;
        if (length_code > remaining_bytes)
        {
            qWarning("Missing data in the data array! Bailing...");
            return std::nullopt;
        }

        for (uint8_t i = length_code; i != 0; --i)
        {
            const uint8_t data_byte{static_cast<uint8_t>(data[data_index++])};
            if (data_byte == 0)
            {
                qWarning("0x00 byte in the data array! Bailing...");
                return std::nullopt;
            }
            output[output_index++] = static_cast<int8_t>(data_byte);
        }

        if (data_index == data_size)
        {
            break;
        }

        // NOLINTNEXTLINE(*-magic-numbers)
        if (length_code != 0xFE)
        {
            output[output_index++] = 0;
        }
    }

    output.resize(output_index);
    return output;
}
}  // namespace server::cobs