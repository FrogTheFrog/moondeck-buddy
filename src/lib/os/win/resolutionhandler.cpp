// header file include
#include "resolutionhandler.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
ResolutionHandler::~ResolutionHandler()
{
    restoreResolution();
}

//---------------------------------------------------------------------------------------------------------------------

bool ResolutionHandler::changeResolution(uint width, uint height)
{
    clearPendingResolution();

    qDebug("Trying to change resolution.");
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
            qDebug("Failed to get display settings for %s.", qUtf8Printable(device_name));
            continue;
        }

        if (devmode.dmPelsWidth == width && devmode.dmPelsHeight == height)
        {
            qDebug("Display %s resolution is already set - skipping.", qUtf8Printable(device_name));
            continue;
        }

        // NOLINTNEXTLINE(*-implicit-bool-conversion,*-signed-bitwise)
        const bool valid_display{(display_device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
                                 // NOLINTNEXTLINE(*-implicit-bool-conversion,*-signed-bitwise)
                                 && (display_device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)};
        if (!valid_display)
        {
            qDebug("Display %s does not have valid flags: %lu.", qUtf8Printable(device_name),
                   display_device.StateFlags);
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

            qInfo("Changed resolution for %s.", qUtf8Printable(device_name));
            if (!m_original_resolutions.contains(device_name))
            {
                m_original_resolutions[device_name] = previous_resolution;
            }
        }
        else
        {
            qInfo("Failed to change resolution for %s. Error code: , %li.", qUtf8Printable(device_name), result);
        }
    }

    return changed_at_least_one;
}

//---------------------------------------------------------------------------------------------------------------------

void ResolutionHandler::restoreResolution()
{
    if (m_original_resolutions.empty())
    {
        return;
    }

    qDebug("Trying to restore resolution.");
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
            qDebug("Failed to get display settings for %s.", qUtf8Printable(device_name));
            continue;
        }

        if (devmode.dmPelsWidth == resolution_it->second.m_width
            && devmode.dmPelsHeight == resolution_it->second.m_height)
        {
            qDebug("Display %s resolution is already restored - skipping.", qUtf8Printable(device_name));
            continue;
        }

        devmode.dmPelsWidth  = resolution_it->second.m_width;
        devmode.dmPelsHeight = resolution_it->second.m_height;
        devmode.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;

        const LONG result =
            ChangeDisplaySettingsExW(static_cast<WCHAR*>(display_device.DeviceName), &devmode, nullptr, 0, nullptr);
        if (result == DISP_CHANGE_SUCCESSFUL)
        {
            qInfo("Restored resolution for %s.", qUtf8Printable(device_name));
            m_original_resolutions.erase(resolution_it);
        }
        else
        {
            qInfo("Failed to restore resolution for %s. Error code: , %li.", qUtf8Printable(device_name), result);
        }
    }

    if (!m_original_resolutions.empty())
    {
        qWarning("Failed to restore resolution for:");
        for (const auto& item : m_original_resolutions)
        {
            qWarning("  * %s", qUtf8Printable(item.first));
        }
        m_original_resolutions.clear();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void ResolutionHandler::setPendingResolution(uint width, uint height)
{
    qDebug("Setting a pending resolution.");
    m_pending_change = {width, height};
}

//---------------------------------------------------------------------------------------------------------------------

void ResolutionHandler::applyPendingChange()
{
    if (m_pending_change)
    {
        qDebug("Applying pending resolution.");
        changeResolution(m_pending_change->m_width, m_pending_change->m_height);
    }
}

//---------------------------------------------------------------------------------------------------------------------

void ResolutionHandler::clearPendingResolution()
{
    if (m_pending_change)
    {
        qDebug("Clearing pending resolution.");
        m_pending_change = std::nullopt;
    }
}
}  // namespace os
