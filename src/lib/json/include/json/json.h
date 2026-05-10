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
template<typename T, glz::opts Opts = {}>
std::expected<T, QString> fromJson(const QString& json_string)
{
    T          value{};
    const auto std_string = json_string.toStdString();
    auto       error_ctx{glz::read<Opts, T>(value, std_string)};
    if (!error_ctx)
    {
        return value;
    }

    return std::unexpected(QString::fromStdString(glz::format_error(error_ctx, std_string)));
}

template<typename T, glz::opts Opts = {}>
std::expected<QString, QString> toJson(const T& value)
{
    auto json_string{glz::write<Opts>(value)};
    if (json_string)
    {
        return QString::fromStdString(json_string.value());
    }

    return std::unexpected(QString::fromStdString(glz::format_error(json_string)));
}
}  // namespace json
