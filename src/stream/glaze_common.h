#pragma once

#include "glaze/glaze.hpp"

namespace av::glz
{
template<typename T, typename... U>
concept IsAnyOf = (std::same_as<T, U> || ...);

//! This should be inherited by any glz::meta<> specialization
//! to automatically remove the "m_" prefix (if available) from the structure
//! keys' names.
struct AdbCompliantKeys
{
    static constexpr std::string_view rename_key(const auto key)
    {
        return (key.size() > 2 && key.substr(0, 2) == "m_") ? key.substr(2) : key;
    }
};
}  // namespace av::glz
