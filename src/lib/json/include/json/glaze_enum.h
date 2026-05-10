#pragma once

// system/Qt includes
#include <glaze/json.hpp>

namespace glz
{
template<typename T>
    requires std::is_enum_v<T>
struct from<JSON, T>
{
    template<auto Opts>
    static void op(T& value, is_context auto&& ctx, auto&& it, auto&& end)
    {
        auto it_orig  = it;
        auto end_orig = end;

        std::string string_value;
        parse<JSON>::op<Opts>(string_value, ctx, it, end);

        bool ok        = false;
        auto key_value = QMetaEnum::fromType<T>().keyToValue(string_value.data(), &ok);
        if (ok)
        {
            value = static_cast<T>(key_value);
        }
        else
        {
            it                       = it_orig;
            end                      = end_orig;
            ctx.error                = error_code::unexpected_enum;
            ctx.custom_error_message = "failed to parse enum value";
        }
    }
};

template<typename T>
    requires std::is_enum_v<T>
struct to<JSON, T>
{
    template<auto Opts>
    static void op(const T& value, is_context auto&& ctx, auto&& b, auto&& ix) noexcept
    {
        auto* string_value = QMetaEnum::fromType<T>().valueToKey(static_cast<quint64>(value));
        if (string_value)
        {
            serialize<JSON>::op<Opts>(string_value, ctx, b, ix);
        }
        else
        {
            ctx.error                = error_code::unexpected_enum;
            ctx.custom_error_message = "failed to serialize enum value";
        }
    }
};

template<typename T>
    requires std::is_enum_v<T>
struct meta<T>
{
    using mimic = std::string;

    static constexpr auto custom_read  = true;
    static constexpr auto custom_write = true;
};
}  // namespace glz