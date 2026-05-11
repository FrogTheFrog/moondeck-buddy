#pragma once

// local includes
#include "glaze_enum.h"
#include "glaze_qregularexpression.h"
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
namespace internal
{
QString tryPartialReadFromFile(const QString& filepath);
void    saveToFile(const QString& filepath, const QString& value);
}  // namespace internal

struct FromJsonOpts
{
    bool m_allow_unknown_keys{false};
    bool m_allow_missing_keys{false};
};

template<typename T, FromJsonOpts Opts = {}>
std::expected<T, QString> fromJson(const QString& json_string)
{
    struct OptsProxy : glz::opts
    {
        consteval OptsProxy()
            : opts{.error_on_unknown_keys = !Opts.m_allow_unknown_keys,
                   .error_on_missing_keys = !Opts.m_allow_missing_keys}
        {
        }
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

template<typename T>
T tryPartialReadFromFile(const QString& filepath)
{
    if (const auto data{internal::tryPartialReadFromFile(filepath)}; !data.isEmpty())
    {
        const auto parsed{json::fromJson<T, {.m_allow_unknown_keys = true, .m_allow_missing_keys = true}>(data)};
        if (!parsed)
        {
            qFatal("Failed to decode JSON data from \"%s\"! Reason:\n%s", qUtf8Printable(filepath),
                   qUtf8Printable(parsed.error()));
        }

        return std::move(parsed.value());
    }

    return T{};
}

template<typename T>
void saveToFile(const QString& filepath, const T& value)
{
    const auto serialized{json::toJson<{.m_indentation = 4}>(value)};
    if (!serialized)
    {
        qFatal("Failed to encode JSON data for \"%s\"! Reason:\n%s", qUtf8Printable(filepath),
               qUtf8Printable(serialized.error()));
    }

    internal::saveToFile(filepath, serialized.value());
}
}  // namespace json
