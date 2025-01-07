#pragma once

// system/Qt includes
#include <QJsonObject>
#include <QJsonValue>
#include <QMetaEnum>

namespace utils
{

template<class T>
struct JsonValueConverter;

template<class T>
    requires std::is_enum_v<T>
struct JsonValueConverter<T>
{
    static std::optional<T> convert(const QJsonValue& json)
    {
        if (!json.isString())
        {
            return std::nullopt;
        }

        bool      success{false};
        auto&&    metaEnum = QMetaEnum::fromType<T>();
        const int enum_value{metaEnum.keyToValue(json.toString().toUtf8(), &success)};
        if (!success)
        {
            return std::nullopt;
        }

        return static_cast<T>(enum_value);
    }
};

template<>
struct JsonValueConverter<QString>
{
    static std::optional<QString> convert(const QJsonValue& json)
    {
        return json.isString() ? std::make_optional(json.toString()) : std::nullopt;
    }
};

template<>
struct JsonValueConverter<bool>
{
    static std::optional<bool> convert(const QJsonValue& json)
    {
        return json.isBool() ? std::make_optional(json.toBool()) : std::nullopt;
    }
};

template<>
struct JsonValueConverter<int>
{
    static std::optional<int> convert(const QJsonValue& json, int min = std::numeric_limits<int>::lowest(),
                                      int max = std::numeric_limits<int>::max())
    {
        if (json.isDouble())
        {
            const int number{json.toInt()};
            if (number >= min && number <= max)
            {
                return number;
            }
        }

        return std::nullopt;
    }
};

template<>
struct JsonValueConverter<uint>
{
    static std::optional<uint> convert(const QJsonValue& json, uint min = 0,
                                       uint max = static_cast<uint>(std::numeric_limits<int>::max()))
    {
        if (min <= static_cast<uint>(std::numeric_limits<int>::max())
            && max <= static_cast<uint>(std::numeric_limits<int>::max()))
        {
            const auto int_number{JsonValueConverter<int>::convert(json, static_cast<int>(min), static_cast<int>(max))};
            if (int_number)
            {
                return static_cast<uint>(*int_number);
            }
        }

        return std::nullopt;
    }
};

template<class T>
std::optional<T> getJsonValue(const QJsonObject& json, const char* field, auto&&... args)
{
    const auto field_value{json.value(field)};
    return JsonValueConverter<T>::convert(field_value, std::forward<decltype(args)>(args)...);
}

template<class T>
std::optional<std::optional<T>> getNullableJsonValue(const QJsonObject& json, const char* field, auto&&... args)
{
    const auto field_value{json.value(field)};
    return field_value.isNull() ? std::make_optional(std::optional<T>())
                                : JsonValueConverter<T>::convert(field_value, std::forward<decltype(args)>(args)...);
}
}  // namespace utils
