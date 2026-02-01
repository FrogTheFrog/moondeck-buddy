#pragma once

#include "glaze_common.h"

namespace av::glz
{
template<template<typename> typename Container, typename V>
concept IsGlazeQListLike = IsAnyOf<Container<V>, QList<V>, QVector<V>>;

template<typename T>
struct GlazeQListLikeProxy
{
    using value_type = T::value_type;
    using iterator   = T::iterator;
    using reference  = T::reference;

    template<typename... Args>
    reference emplace_back(Args... args)
    {
        return m_container.emplace_back(std::forward<Args>(args)...);
    }

    iterator begin()
    {
        return iterator{m_container.begin()};
    }
    iterator end()
    {
        return iterator{m_container.end()};
    }

    std::size_t size() const
    {
        return static_cast<std::size_t>(m_container.size());
    }

    T& m_container;
};

template<typename T>
struct GlazeQListLikeProxyConst
{
    using value_type = T::value_type;
    using iterator   = T::const_iterator;

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
template<template<typename> typename Container, typename V>
    requires av::glz::IsGlazeQListLike<Container, V>
struct from<JSON, Container<V>>
{
    template<auto Opts>
    static void op(Container<V>& value, is_context auto&& ctx, auto&& it, auto&& end)
    {
        av::glz::GlazeQListLikeProxy proxy{value};
        parse<JSON>::op<Opts>(proxy, ctx, it, end);
    }
};

template<template<typename> typename Container, typename V>
    requires av::glz::IsGlazeQListLike<Container, V>
struct to<JSON, Container<V>>
{
    template<auto Opts>
    static void op(const Container<V>& value, is_context auto&& ctx, auto&& b, auto&& ix) noexcept
    {
        av::glz::GlazeQListLikeProxyConst proxy{value};
        serialize<JSON>::op<Opts>(proxy, ctx, b, ix);
    }
};
}  // namespace glz
