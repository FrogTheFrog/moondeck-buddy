#pragma once

#include "glaze/glaze.hpp"
#include <QMetaEnum>

namespace av::glz
{
template<typename T>
constexpr bool checkIsQtEnum()
{
    // Constrain required the value to be of type bool, but this is
    // a truthy enum, therefor we need this workaround...
    if constexpr (QtPrivate::IsQEnumHelper<T>::Value)
    {
        return true;
    }
    else
    {
        return false;
    }
}

template<typename T>
concept IsQtEnum = checkIsQtEnum<T>();

}  // namespace av::glz

namespace glz
{
template<typename T>
    requires av::glz::IsQtEnum<T>
struct from<JSON, T>
{
    template<auto Opts>
    static void op(T& value, is_context auto&& ctx, auto&& it, auto&& end)
    {
        // Preserve the iterators in case of an enum parse error (for pretty formatting)
        auto it_orig  = it;
        auto end_orig = end;

        // We need null terminated string, therefore using std::string instead of view.
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
            ctx.custom_error_message = "failed to parse Q_ENUM value";
        }
    }
};

template<typename T>
    requires av::glz::IsQtEnum<T>
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
            ctx.custom_error_message = "failed to serialize Q_ENUM value";
        }
    }
};

template<typename T>
    requires av::glz::IsQtEnum<T>
struct meta<T>
{
    // We must inform Glaze that our custom type mimics string so that Glaze does not add unnecessary quotation marks
    // when it is used as a key.
    using mimic = std::string;

    // Glaze already has a handler for enum, therefore we need to tell it that we are also partially handling
    // some of the enum cases to avoid confusion.
    static constexpr auto custom_read  = true;
    static constexpr auto custom_write = true;
};
}  // namespace glz
