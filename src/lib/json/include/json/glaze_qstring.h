#pragma once

// system/Qt includes
#include <QString>
#include <glaze/json.hpp>

namespace glz
{
template<>
struct from<JSON, QString>
{
    template<auto Opts>
    static void op(QString& value, is_context auto&& ctx, auto&& it, auto&& end)
    {
        std::string string_value;
        parse<JSON>::op<Opts>(string_value, ctx, it, end);
        value = QString::fromStdString(string_value);
    }
};

template<>
struct to<JSON, QString>
{
    template<auto Opts>
    static void op(const QString& value, is_context auto&& ctx, auto&& b, auto&& ix) noexcept
    {
        const QByteArray utf8{value.toUtf8()};
        serialize<JSON>::op<Opts>(std::string_view{utf8.constData(), static_cast<std::size_t>(utf8.size())}, ctx, b,
                                  ix);
    }
};

template<>
struct meta<QString>
{
    using mimic = std::string;
};
}  // namespace glz