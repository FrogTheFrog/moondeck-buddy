#pragma once

// system/Qt includes
#include <QRegularExpression>
#include <glaze/json.hpp>

namespace glz
{
template<>
struct from<JSON, QRegularExpression>
{
    template<auto Opts>
    static void op(QRegularExpression& value, is_context auto&& ctx, auto&& it, auto&& end)
    {
        auto it_orig  = it;
        auto end_orig = end;

        QString string_value;
        parse<JSON>::op<Opts>(string_value, ctx, it, end);

        value = QRegularExpression{string_value};
        if (!value.isValid())
        {
            it                       = it_orig;
            end                      = end_orig;
            ctx.error                = error_code::parse_error;
            ctx.custom_error_message = "failed to parse regular expression value";
        }
    }
};

template<>
struct to<JSON, QRegularExpression>
{
    template<auto Opts>
    static void op(const QRegularExpression& value, is_context auto&& ctx, auto&& b, auto&& ix) noexcept
    {
        if (value.isValid())
        {
            serialize<JSON>::op<Opts>(value.pattern(), ctx, b, ix);
        }
        else
        {
            ctx.error                = error_code::unexpected_enum;
            ctx.custom_error_message = "failed to serialize regular expression value";
        }
    }
};

template<>
struct meta<QRegularExpression>
{
    using mimic = std::string;
};
}  // namespace glz