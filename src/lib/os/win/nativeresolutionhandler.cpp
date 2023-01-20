// header file include
#include "nativeresolutionhandler.h"

// A SEPARATE WINDOWS INCLUDE BECAUSE OF THE SMELL!
#include <windows.h>

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
// NOLINTNEXTLINE(*-cognitive-complexity)
NativeResolutionHandler::ChangedResMap NativeResolutionHandler::changeResolution(const DisplayPredicate& predicate)
{
    Q_ASSERT(predicate);

    ChangedResMap changed_res;
    for (int i = 0;; ++i)
    {
        DISPLAY_DEVICEW display_device;
        ZeroMemory(&display_device, sizeof(display_device));
        display_device.cb = sizeof(display_device);

        if (EnumDisplayDevicesW(nullptr, i, &display_device, EDD_GET_DEVICE_INTERFACE_NAME) == FALSE)
        {
            break;
        }

        static_assert(sizeof(wchar_t) == sizeof(char16_t), "Wide char is not 2 bytes :/");
        // NOLINTNEXTLINE(*-reinterpret-cast)
        const QString device_name{QString::fromUtf16(reinterpret_cast<char16_t*>(display_device.DeviceName))};

        DEVMODE devmode;
        ZeroMemory(&devmode, sizeof(devmode));
        devmode.dmSize = sizeof(devmode);

        if (EnumDisplaySettingsW(static_cast<WCHAR*>(display_device.DeviceName), ENUM_CURRENT_SETTINGS, &devmode)
            == FALSE)
        {
            qCDebug(lc::os) << "Failed to get display settings for" << device_name;
            continue;
        }

        // NOLINTNEXTLINE(*-implicit-bool-conversion,*-signed-bitwise)
        const bool is_primary{static_cast<bool>(display_device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)};
        const auto resolution{predicate(device_name, is_primary)};

        if (!resolution)
        {
            qCDebug(lc::os).nospace() << "Display " << device_name << " was skipped by predicate.";
            continue;
        }

        if (devmode.dmPelsWidth == resolution->m_width && devmode.dmPelsHeight == resolution->m_height)
        {
            qCDebug(lc::os).nospace() << "Display " << device_name << " resolution is already set - skipping.";
            changed_res[device_name] = std::nullopt;
            continue;
        }

        const Resolution previous_resolution{devmode.dmPelsWidth, devmode.dmPelsHeight};

        devmode.dmPelsWidth  = resolution->m_width;
        devmode.dmPelsHeight = resolution->m_height;
        devmode.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;

        const LONG result =
            ChangeDisplaySettingsExW(static_cast<WCHAR*>(display_device.DeviceName), &devmode, nullptr, 0, nullptr);
        if (result == DISP_CHANGE_SUCCESSFUL)
        {
            qCDebug(lc::os) << "Changed resolution for" << device_name;
            changed_res[device_name] = previous_resolution;
        }
        else
        {
            qCWarning(lc::os).nospace() << "Failed to change resolution for " << device_name
                                        << ". Error code: " << result;
        }
    }

    return changed_res;
}
}  // namespace os
