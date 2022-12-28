// header file include
#include "resolutionhandler.h"

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
ResolutionHandler::~ResolutionHandler()
{
    restoreResolution();
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-cognitive-complexity)
bool ResolutionHandler::changeResolution(uint width, uint height)
{
    clearPendingResolution();

    qCDebug(lc::os) << "Trying to change resolution.";
    bool changed_at_least_one{false};
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

        if (devmode.dmPelsWidth == width && devmode.dmPelsHeight == height)
        {
            qCDebug(lc::os).nospace() << "Display " << device_name << " resolution is already set - skipping.";
            continue;
        }

        // NOLINTNEXTLINE(*-implicit-bool-conversion,*-signed-bitwise)
        const bool valid_display{(display_device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
                                 // NOLINTNEXTLINE(*-implicit-bool-conversion,*-signed-bitwise)
                                 && (display_device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)};
        if (!valid_display)
        {
            qCDebug(lc::os).nospace() << "Display " << device_name
                                      << " does not have valid flags: " << display_device.StateFlags;
            continue;
        }

        const Resolution previous_resolution{devmode.dmPelsWidth, devmode.dmPelsHeight};

        devmode.dmPelsWidth  = width;
        devmode.dmPelsHeight = height;
        devmode.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;

        const LONG result =
            ChangeDisplaySettingsExW(static_cast<WCHAR*>(display_device.DeviceName), &devmode, nullptr, 0, nullptr);
        if (result == DISP_CHANGE_SUCCESSFUL)
        {
            changed_at_least_one = true;

            qCInfo(lc::os) << "Changed resolution for" << device_name;
            if (!m_original_resolutions.contains(device_name))
            {
                m_original_resolutions[device_name] = previous_resolution;
            }
        }
        else
        {
            qCWarning(lc::os).nospace() << "Failed to change resolution for " << device_name
                                        << ". Error code: " << result;
        }
    }

    return changed_at_least_one;
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-cognitive-complexity)
void ResolutionHandler::restoreResolution()
{
    if (m_original_resolutions.empty())
    {
        return;
    }

    qCDebug(lc::os) << "Trying to restore resolution.";
    for (int i = 0;; ++i)
    {
        if (m_original_resolutions.empty())
        {
            break;
        }

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

        const auto resolution_it = m_original_resolutions.find(device_name);
        if (resolution_it == std::cend(m_original_resolutions))
        {
            continue;
        }

        DEVMODE devmode;
        ZeroMemory(&devmode, sizeof(devmode));
        devmode.dmSize = sizeof(devmode);

        if (EnumDisplaySettingsW(static_cast<WCHAR*>(display_device.DeviceName), ENUM_CURRENT_SETTINGS, &devmode)
            == FALSE)
        {
            qCDebug(lc::os) << "Failed to get display settings for" << device_name;
            continue;
        }

        if (devmode.dmPelsWidth == resolution_it->second.m_width
            && devmode.dmPelsHeight == resolution_it->second.m_height)
        {
            qCDebug(lc::os).nospace() << "Display " << device_name << " resolution is already restored - skipping.";
            m_original_resolutions.erase(resolution_it);
            continue;
        }

        devmode.dmPelsWidth  = resolution_it->second.m_width;
        devmode.dmPelsHeight = resolution_it->second.m_height;
        devmode.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;

        const LONG result =
            ChangeDisplaySettingsExW(static_cast<WCHAR*>(display_device.DeviceName), &devmode, nullptr, 0, nullptr);
        if (result == DISP_CHANGE_SUCCESSFUL)
        {
            qCInfo(lc::os) << "Changed resolution for" << device_name;
            m_original_resolutions.erase(resolution_it);
        }
        else
        {
            qCWarning(lc::os).nospace() << "Failed to restore resolution for " << device_name
                                        << ". Error code: " << result;
        }
    }

    if (!m_original_resolutions.empty())
    {
        qCWarning(lc::os) << "Failed to restore resolution for:";
        for (const auto& item : m_original_resolutions)
        {
            qCWarning(lc::os) << "  *" << item.first;
        }
        m_original_resolutions.clear();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void ResolutionHandler::setPendingResolution(uint width, uint height)
{
    qCDebug(lc::os) << "Setting a pending resolution.";
    m_pending_change = {width, height};
}

//---------------------------------------------------------------------------------------------------------------------

void ResolutionHandler::applyPendingChange()
{
    if (m_pending_change)
    {
        qCDebug(lc::os) << "Applying pending resolution.";
        changeResolution(m_pending_change->m_width, m_pending_change->m_height);
    }
}

//---------------------------------------------------------------------------------------------------------------------

void ResolutionHandler::clearPendingResolution()
{
    if (m_pending_change)
    {
        qCDebug(lc::os) << "Clearing pending resolution.";
        m_pending_change = std::nullopt;
    }
}
}  // namespace os
