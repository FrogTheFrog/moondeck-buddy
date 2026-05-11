#pragma once

// local includes
#include "glaze_enum.h"
#include "glaze_qstring.h"

namespace glz
{
template<typename>
struct meta
{
    static constexpr std::string_view rename_key(const auto key)
    {
        return (key.size() > 2 && key.substr(0, 2) == "m_") ? key.substr(2) : key;
    }
};
}  // namespace glz

namespace json
{
struct FromJsonOpts
{
};

template<typename T, FromJsonOpts Opts = {}>
std::expected<T, QString> fromJson(const QString& json_string)
{
    struct OptsProxy : glz::opts
    {
    };

    T          value{};
    const auto std_string = json_string.toStdString();
    auto       error_ctx{glz::read<OptsProxy{}, T>(value, std_string)};
    if (!error_ctx)
    {
        return value;
    }

    return std::unexpected(QString::fromStdString(glz::format_error(error_ctx, std_string)));
}

struct ToJsonOpts
{
    bool    m_keep_null_members{true};
    uint8_t m_indentation{0};
};

template<typename T, ToJsonOpts Opts = {}>
std::expected<QString, QString> toJson(const T& value)
{
    struct OptsProxy : glz::opts
    {
        consteval OptsProxy()
            : opts{.skip_null_members = !Opts.m_keep_null_members, .prettify = Opts.m_indentation > 0}
        {
        }

        uint8_t indentation_width = Opts.m_indentation;
    };

    auto json_string{glz::write<OptsProxy{}>(value)};
    if (json_string)
    {
        return QString::fromStdString(json_string.value());
    }

    return std::unexpected(QString::fromStdString(glz::format_error(json_string)));
}

template<ToJsonOpts Opts, typename T>
std::expected<QString, QString> toJson(const T& value)
{
    return toJson<T, Opts>(value);
}
}  // namespace json
