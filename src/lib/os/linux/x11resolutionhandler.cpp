// header file include
#include "os/linux/x11resolutionhandler.h"

// local includes
#include "shared/loggingcategories.h"

// X11 smelly includes
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
using Resolution = os::NativeResolutionHandlerInterface::Resolution;

//---------------------------------------------------------------------------------------------------------------------

int findMatchingSize(const Resolution& resolution, const XRRScreenSize* sizes, int number_of_sizes)
{
    const int not_found{-1};
    if (sizes == nullptr)
    {
        return not_found;
    }

    for (int i = 0; i < number_of_sizes; ++i)
    {
        // NOLINTNEXTLINE(*-pointer-arithmetic)
        const auto& size{sizes[i]};

        if (static_cast<uint>(size.width) == resolution.m_width
            && static_cast<uint>(size.height) == resolution.m_height)
        {
            return i;
        }
    }

    return not_found;
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
// NOLINTNEXTLINE(*-cognitive-complexity)
X11ResolutionHandler::ChangedResMap X11ResolutionHandler::changeResolution(const DisplayPredicate& predicate)
{
    Q_ASSERT(predicate);

    Display*   display{nullptr};
    const auto display_cleanup{qScopeGuard(
        [&]()
        {
            if (display != nullptr)
            {
                XCloseDisplay(display);
            }
        })};

    display = XOpenDisplay(nullptr);
    if (display == nullptr)
    {
        qCDebug(lc::os) << "No default X11 display is available.";
        return {};
    }

    ChangedResMap changed_res;
    const int     screen_count{XScreenCount(display)};
    const int     primary_screen{XDefaultScreen(display)};
    for (int screen = 0; screen < screen_count; ++screen)
    {
        const QString screen_name{QString::number(screen)};
        const bool    is_primary{screen == primary_screen};

        const auto resolution{predicate(screen_name, is_primary)};
        if (!resolution)
        {
            qCDebug(lc::os).nospace() << "Screen " << screen_name << " was skipped by predicate.";
            continue;
        }

        const auto  root_window = XRootWindow(display, screen);
        auto* const screen_info = XRRGetScreenInfo(display, root_window);
        const auto  screen_info_cleanup{qScopeGuard(
            [&]()
            {
                if (screen_info != nullptr)
                {
                    XRRFreeScreenConfigInfo(screen_info);
                }
            })};

        if (screen_info == nullptr)
        {
            qCDebug(lc::os) << "XRRGetScreenInfo failed for" << screen_name;
            continue;
        }

        Rotation current_rotation{0};
        auto     current_size_index = XRRConfigCurrentConfiguration(screen_info, &current_rotation);

        // all_screen_sizes ptr belongs to screen_info, no need for a cleanup
        int         number_of_sizes{0};
        const auto* all_screen_sizes = XRRConfigSizes(screen_info, &number_of_sizes);

        if (all_screen_sizes == nullptr || number_of_sizes == 0)
        {
            qCDebug(lc::os) << "XRRConfigSizes failed for" << screen_name;
            continue;
        }

        if (current_size_index >= number_of_sizes)
        {
            qCDebug(lc::os) << "current_size_index is out of range for" << screen_name;
            continue;
        }

        // NOLINTNEXTLINE(*-pointer-arithmetic)
        const Resolution previous_resolution{static_cast<uint>(all_screen_sizes[current_size_index].width),
                                             // NOLINTNEXTLINE(*-pointer-arithmetic)
                                             static_cast<uint>(all_screen_sizes[current_size_index].height)};
        if (previous_resolution.m_height == resolution->m_height && previous_resolution.m_width == resolution->m_width)
        {
            qCDebug(lc::os) << "Screen" << screen_name << "already has the requested resolution - skipping.";
            changed_res[screen_name] = std::nullopt;
            continue;
        }

        const auto matching_size_index{findMatchingSize(*resolution, all_screen_sizes, number_of_sizes)};
        if (matching_size_index < 0)
        {
            qCDebug(lc::os) << "Matching size not found for" << screen_name;
            continue;
        }

        const auto result =
            XRRSetScreenConfig(display, screen_info, root_window, matching_size_index, current_rotation, CurrentTime);
        if (result != RRSetConfigSuccess)
        {
            qCWarning(lc::os) << "Failed to change resolution for" << screen_name;
            continue;
        }

        qCDebug(lc::os) << "Changed resolution for" << screen_name;
        changed_res[screen_name] = previous_resolution;
    }

    return changed_res;
}
}  // namespace os
