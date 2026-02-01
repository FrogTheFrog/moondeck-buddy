#pragma once

#include "glaze/glaze.hpp"
#include <QString>

//! A proxy-like object for allowing Glaze to directly
//! work with QString as if it was a std::string, thus avoiding
//! unnecessary string re-allocations.
//!
//! The object tries to satisfy the glz::string_t<T> constrain.
//!
//! TODO: is this a good idea? Are we bypassing utf-8 checks of smt in QString?
// struct GlazeQStringLikeProxy
// {
//     operator std::string_view() const { return std::string_view{m_qt_string.constData(), m_qt_string.size()}; }
//
//     QString& m_qt_string;
// };

namespace glz
{
template<>
struct from<JSON, QString>
{
    template<auto Opts>
    static void op(QString& value, is_context auto&& ctx, auto&& it, auto&& end)
    {
        // QString stores the data in UTF-16 format so there is no easy
        // way to directly store the UTF-8 string inside a QString since
        // we would have to do UTF conversion anyway. Thus, it's best
        // to use reuse the std::string implementation directly.
        //
        // We cannot use std::string_view, because the strings might contain
        // escaped characters and glaze would be unable to "fix" them since
        // it cannot allocate the buffer.
        std::string proxy;
        parse<JSON>::op<Opts>(proxy, ctx, it, end);
        value = QString::fromStdString(proxy);
    }
};

template<>
struct to<JSON, QString>
{
    template<auto Opts>
    static void op(const QString& value, is_context auto&& ctx, auto&& buffer, auto&& ix) noexcept
    {
        serialize<JSON>::op<Opts>(value.toUtf8(), ctx, buffer, ix);
    }
};

template<>
struct meta<QString>
{
    // We must inform Glaze that our custom type mimics string so that Glaze does not add unnecessary quotation marks
    // when it is used as a key.
    using mimic = std::string;
};
}  // namespace glz