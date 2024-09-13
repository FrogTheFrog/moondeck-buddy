// header file include
#include "os/linux/nativeresolutionhandler.h"

// local includes
#include "shared/loggingcategories.h"

namespace
{
bool isWaylandSession()
{
    const auto xdg_session_env = qgetenv("XDG_SESSION_TYPE");
    if (!xdg_session_env.isEmpty())
    {
        const QString xdg_session_str{QString(xdg_session_env).toLower()};
        if (xdg_session_str == QStringLiteral("wayland"))
        {
            qCDebug(lc::os) << "XDG_SESSION_TYPE says it's a Wayland session!";
            return true;
        }

        if (xdg_session_str == QStringLiteral("x11"))
        {
            qCDebug(lc::os) << "XDG_SESSION_TYPE says it's an X11 session!";
            return false;
        }

        qCDebug(lc::os).nospace() << "XDG_SESSION_TYPE has unknown value (" << xdg_session_env
                                  << "). Checking for WAYLAND_DISPLAY!";
    }
    else
    {
        qCDebug(lc::os) << "XDG_SESSION_TYPE not present in the ENV. Checking for WAYLAND_DISPLAY!";
    }

    const auto wayland_display_env = qgetenv("WAYLAND_DISPLAY");
    if (!wayland_display_env.isEmpty())
    {
        qCDebug(lc::os) << "Found WAYLAND_DISPLAY, assuming Wayland session.";
        return true;
    }

    qCWarning(lc::os) << "No ENV found to determine session type, assuming X11 session.";
    return false;
}
}  // namespace

namespace os
{
NativeResolutionHandler::ChangedResMap NativeResolutionHandler::changeResolution(const DisplayPredicate& predicate)
{
    if (isWaylandSession())
    {
        qCDebug(lc::os) << "Resolution change for Wayland session is not supported yet...";
        return {};
    }

    return x11_handler.changeResolution(predicate);
}
}  // namespace os
