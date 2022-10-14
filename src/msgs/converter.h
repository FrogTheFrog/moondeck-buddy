#pragma once

// system/Qt includes
#include <QJsonDocument>
#include <variant>

//---------------------------------------------------------------------------------------------------------------------

namespace converter
{
namespace internal
{
namespace from
{
template<class VariantType, class... T>
struct ConverterHelper;

//---------------------------------------------------------------------------------------------------------------------

template<class VariantType, class First, class... Rest>
struct ConverterHelper<VariantType, First, Rest...>
{
    static bool convert(const QJsonDocument& data, VariantType& variant)
    {
        return ConverterHelper<VariantType, First>::convert(data, variant)
               || ConverterHelper<VariantType, Rest...>::convert(data, variant);
    }
};

//---------------------------------------------------------------------------------------------------------------------

template<class VariantType, class T>
struct ConverterHelper<VariantType, T>
{
    static bool convert(const QJsonDocument& data, VariantType& variant)
    {
        std::optional<T> converted_data{T::fromJson(data)};
        if (converted_data)
        {
            variant = std::make_optional(std::move(*converted_data));
            return true;
        }
        return false;
    }
};

//---------------------------------------------------------------------------------------------------------------------

template<class VariantType>
struct ConverterHelper<VariantType>
{
    static bool convert(const QJsonDocument&, VariantType&)
    {
        return false;
    }
};
}  // namespace from
}  // namespace internal

//---------------------------------------------------------------------------------------------------------------------

template<class... T>
decltype(auto) fromJson(const QJsonDocument& data)
{
    std::optional<std::variant<T...>> output;
    internal::from::ConverterHelper<decltype(output), T...>::convert(data, output);
    return output;
}

//---------------------------------------------------------------------------------------------------------------------

template<class T>
QJsonDocument toJson(const T& data)
{
    return T::toJson(data);
}
}  // namespace converter