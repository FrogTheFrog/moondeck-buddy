// header file include
#include "resolutionhandler.h"

// system/Qt includes
#include <algorithm>

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const int RETRY_TIME{10};
const int SEC_TO_MS{1000};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
ResolutionHandler::ResolutionHandler(std::unique_ptr<NativeResolutionHandlerInterface> native_handler,
                                     std::set<QString>                                 handled_displays)
    : m_native_handler{std::move(native_handler)}
    , m_handled_displays{std::move(handled_displays)}
{
    Q_ASSERT(m_native_handler != nullptr);

    connect(&m_restore_retry_timer, &QTimer::timeout, this, [this]() { restoreResolution(); });
    m_restore_retry_timer.setInterval(RETRY_TIME * SEC_TO_MS);
    m_restore_retry_timer.setSingleShot(true);
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
    m_restore_retry_timer.stop();

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

    for (const auto& restored : result)
    {
        const auto& key{restored.first};
        m_original_resolutions.erase(key);
    }

    if (!m_original_resolutions.empty())
    {
        qCDebug(lc::os) << "Failed to restore resolution. Trying again in" << RETRY_TIME << "seconds.";
        m_restore_retry_timer.start();
    }
}
}  // namespace os
