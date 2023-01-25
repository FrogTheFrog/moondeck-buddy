// header file include
#include "x11resolutionhandler.h"

// local includes
#include "shared/loggingcategories.h"

// X11 smelly includes
#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
using Resolution = os::NativeResolutionHandlerInterface::Resolution;

//---------------------------------------------------------------------------------------------------------------------

XF86VidModeModeInfo* findMatchingMode(const Resolution& resolution, XF86VidModeModeInfo** modes, int size)
{
    Q_ASSERT(modes);

    for (int i = 0; i < size; ++i)
    {
        // NOLINTNEXTLINE(*-pointer-arithmetic)
        XF86VidModeModeInfo* mode{modes[i]};
        Q_ASSERT(mode);

        if (mode->hdisplay == resolution.m_width && mode->vdisplay == resolution.m_height)
        {
            return mode;
        }
    }

    return nullptr;
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

        int                 dotclock{0};  // WTF is this?
        XF86VidModeModeLine mode_line{};
        if (XF86VidModeGetModeLine(display, screen, &dotclock, &mode_line) == False)
        {
            qCDebug(lc::os) << "Failed to get XF86VidModeModeLine for" << screen_name;
            continue;
        }

        const Resolution previous_resolution{mode_line.hdisplay, mode_line.vdisplay};
        if (previous_resolution.m_height == resolution->m_height && previous_resolution.m_width == resolution->m_width)
        {
            qCDebug(lc::os) << "Screen" << screen_name << "aslread has the requested resolution - skipping.";
            changed_res[screen_name] = std::nullopt;
            continue;
        }

        int                   modes_size{0};
        XF86VidModeModeInfo** modes{nullptr};
        const auto            modes_cleanup{qScopeGuard(
            [&]()
            {
                if (modes != nullptr)
                {
                    XFree(modes);
                }
            })};

        if (XF86VidModeGetAllModeLines(display, screen, &modes_size, &modes) == False)
        {
            qCWarning(lc::os) << "Failed to get XF86VidModeModeInfo for" << screen_name;
            continue;
        }

        auto* matching_mode{findMatchingMode(*resolution, modes, modes_size)};
        if (matching_mode == nullptr)
        {
            qCDebug(lc::os) << "Failed to get mode with matching resolution for" << screen_name;
            continue;
        }

        if (XF86VidModeSwitchToMode(display, screen, matching_mode) == False)
        {
            qCWarning(lc::os) << "Failed to switch to new mode for" << screen_name;
            continue;
        }

        qCDebug(lc::os) << "Changed mode for" << screen_name;
        changed_res[screen_name] = previous_resolution;

        if (XF86VidModeSetViewPort(display, screen, 0, 0) == False)
        {
            qCWarning(lc::os) << "Failed to set viewport for" << screen_name;
            // No "continue" as we must at least proceed with the flush
        }

        XFlush(display);
    }

    return changed_res;
}
}  // namespace os
