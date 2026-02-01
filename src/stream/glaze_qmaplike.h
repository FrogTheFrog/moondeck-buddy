#pragma once

#include "glaze_common.h"

namespace av::glz
{
template<template<typename, typename> typename Container, typename K, typename V>
concept IsGlazeQMapLike = IsAnyOf<Container<K, V>, QMap<K, V>, QHash<K, V>>;

template<typename T>
struct GlazeQMapLikeProxy
{
    using key_type   = T::key_type;
    using value_type = T::mapped_type;
    using iterator   = T::key_value_iterator;

    value_type& operator[](const key_type& key)
    {
        return m_container[key];
    }

    iterator begin()
    {
        return iterator{m_container.begin()};
    }
    iterator end()
    {
        return iterator{m_container.end()};
    }

    T& m_container;
};

template<typename T>
struct GlazeQMapLikeProxyConst
{
    using key_type   = T::key_type;
    using value_type = T::mapped_type;
    using iterator   = T::const_key_value_iterator;

    const value_type& operator[](const key_type& key) const
    {
        return m_container[key];
    }

    iterator begin() const
    {
        return iterator{m_container.begin()};
    }
    iterator end() const
    {
        return iterator{m_container.end()};
    }

    const T& m_container;
};
}  // namespace av::glz

namespace glz
{
template<template<typename, typename> typename Container, typename K, typename V>
    requires av::glz::IsGlazeQMapLike<Container, K, V>
struct from<JSON, Container<K, V>>
{
    template<auto Opts>
    static void op(Container<K, V>& value, is_context auto&& ctx, auto&& it, auto&& end)
    {
        av::glz::GlazeQMapLikeProxy proxy{value};
        parse<JSON>::op<Opts>(proxy, ctx, it, end);
    }
};

template<template<typename, typename> typename Container, typename K, typename V>
    requires av::glz::IsGlazeQMapLike<Container, K, V>
struct to<JSON, Container<K, V>>
{
    template<auto Opts>
    static void op(const Container<K, V>& value, is_context auto&& ctx, auto&& b, auto&& ix) noexcept
    {
        av::glz::GlazeQMapLikeProxyConst proxy{value};
        serialize<JSON>::op<Opts>(proxy, ctx, b, ix);
    }
};
}  // namespace glz
