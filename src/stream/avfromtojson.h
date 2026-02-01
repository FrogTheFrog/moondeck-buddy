#pragma once

#include "glaze_common.h"
#include "glaze_qenum.h"
#include "glaze_qlistlike.h"
#include "glaze_qmaplike.h"
#include "glaze_qstring.h"

namespace glz
{
//! Default parameters for non-specialized types
template<typename>
struct meta : av::glz::AdbCompliantKeys
{
};
}  // namespace glz

template<typename T>
std::expected<T, QString> AVFromJson(const QString& json_string)
{
    const auto std_string = json_string.toStdString();
    auto       parsed_value{glz::read_json<T>(std_string)};
    if (parsed_value)
    {
        return parsed_value.value();
    }

    return std::unexpected(QString::fromStdString(glz::format_error(parsed_value.error(), std_string)));
}

template<typename T>
std::expected<QString, QString> AVToJson(const T& value, bool pretty = false)
{
    constexpr auto opts_with_no_pretty = glz::opts{.skip_null_members = true};
    constexpr auto opts_with_pretty    = glz::opts{.skip_null_members = true, .prettify = true};

    auto json_string{pretty ? glz::write<opts_with_pretty>(value) : glz::write<opts_with_no_pretty>(value)};
    if (json_string)
    {
        return QString::fromStdString(json_string.value());
    }

    return std::unexpected(QString::fromStdString(glz::format_error(json_string)));
}
