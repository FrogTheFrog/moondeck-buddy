// header file include
#include "resolutionhandler.h"

// system/Qt includes
#include <algorithm>

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
ResolutionHandler::ResolutionHandler(std::unique_ptr<NativeResolutionHandlerInterface> native_handler,
                                     std::set<QString>                                 handled_displays)
    : m_native_handler{std::move(native_handler)}
    , m_handled_displays{std::move(handled_displays)}
{
    Q_ASSERT(m_native_handler != nullptr);
}

//---------------------------------------------------------------------------------------------------------------------

ResolutionHandler::~ResolutionHandler()
{
    restoreResolution();
}

//---------------------------------------------------------------------------------------------------------------------

bool ResolutionHandler::changeResolution(uint width, uint height)
{
    qCDebug(lc::os) << "Trying to change resolution.";

    const auto result{m_native_handler->changeResolution(
        [&](const QString& display_name, bool is_primary) -> std::optional<Resolution>
        {
            if ((m_handled_displays.empty() && is_primary) || m_handled_displays.contains(display_name))
            {
                if (is_primary)
                {
                    return Resolution{width, height};
                }
            }

            return std::nullopt;
        })};

    if (result.empty())
    {
        return false;
    }

    for (const auto& item : result)
    {
        const QString& display_name{item.first};
        const auto&    maybe_changed_resolution{item.second};

        if (!maybe_changed_resolution)
        {
            // Resolution did not change from the previous one since they matched
            continue;
        }

        if (m_original_resolutions.contains(display_name))
        {
            continue;
        }

        m_original_resolutions[display_name] = *maybe_changed_resolution;
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

void ResolutionHandler::restoreResolution()
{
    if (m_original_resolutions.empty())
    {
        return;
    }

    qCDebug(lc::os) << "Trying to restore resolution.";
    const auto result{m_native_handler->changeResolution(
        [&](const QString& display_name, bool is_primary) -> std::optional<Resolution>
        {
            Q_UNUSED(is_primary)
            if (m_original_resolutions.contains(display_name))
            {
                return m_original_resolutions[display_name];
            }

            return std::nullopt;
        })};

    std::set<QString> original_keys;
    std::transform(std::cbegin(m_original_resolutions), std::cend(m_original_resolutions),
                   std::inserter(original_keys, std::begin(original_keys)),
                   [](const auto& item) { return item.first; });

    std::set<QString> result_keys;
    std::transform(std::cbegin(result), std::cend(result), std::inserter(result_keys, std::begin(result_keys)),
                   [](const auto& item) { return item.first; });

    std::set<QString> diff_keys;
    std::set_difference(std::cbegin(original_keys), std::cend(original_keys), std::cbegin(result_keys),
                        std::cend(result_keys), std::inserter(diff_keys, std::begin(diff_keys)));

    if (!diff_keys.empty())
    {
        qCWarning(lc::os) << "Failed to restore resolution for:";
        for (const auto& name : diff_keys)
        {
            qCWarning(lc::os) << "  *" << name;
        }
    }

    m_original_resolutions.clear();
}
}  // namespace os
