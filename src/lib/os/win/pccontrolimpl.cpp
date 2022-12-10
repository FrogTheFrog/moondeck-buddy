// header file include
#include "pccontrolimpl.h"

// system/Qt includes
#include <QAbstractNativeEventFilter>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
namespace
{
const QString REG_STEAM_PATH{R"(HKEY_CURRENT_USER\Software\Valve\Steam)"};
const QString REG_STEAM_APPS_PATH{R"(HKEY_CURRENT_USER\Software\Valve\Steam\Apps)"};
const QString REG_APP_ID{"RunningAppId"};
const QString REG_EXEC{"SteamExe"};
const QString REG_APP_RUNNING{"Running"};
const QString REG_APP_UPDATING{"Updating"};
const uint    FALLBACK_APP_ID{0};
const int     EXTRA_DELAY_SECS{10};
const int     MS_TO_SEC{1000};

//---------------------------------------------------------------------------------------------------------------------

std::optional<QString> getLinkLocation()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    if (base.isEmpty())
    {
        return std::nullopt;
    }

    const QFileInfo fileInfo(QCoreApplication::applicationFilePath());
    return base + QDir::separator() + "Startup" + QDir::separator() + fileInfo.completeBaseName() + ".lnk";
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-easily-swappable-parameters)
void setResolution(uint width, uint height)
{
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

        devmode.dmPelsWidth  = width;
        devmode.dmPelsHeight = height;
        devmode.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;

        const LONG result =
            ChangeDisplaySettingsExW(static_cast<WCHAR*>(display_device.DeviceName), &devmode, nullptr, 0, nullptr);
        if (result == DISP_CHANGE_SUCCESSFUL)
        {
            qInfo("Changed resolution for %s.", qUtf8Printable(device_name));
        }
        else
        {
            qInfo("Failed to change resolution for %s. Error code: , %li.", qUtf8Printable(device_name), result);
        }
    }
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

PcControlImpl::PcControlImpl(QString app_name)
    : m_steam_process_tracker{QRegularExpression{R"([\\/]steam\.exe$)", QRegularExpression::CaseInsensitiveOption}}
    , m_message_queue{"MoonDeckBuddyMessageQueueObserver"}
    , m_app_name{std::move(app_name)}
{
    // App ID notification
    connect(&m_reg_key, &RegKey::signalValuesChanged, this, &PcControlImpl::slotHandleRegKeyChanges);
    m_reg_key.open(REG_STEAM_PATH, {REG_APP_ID, REG_EXEC});

    // Exit timer
    m_exit_timer.setSingleShot(true);
    connect(&m_exit_timer, &QTimer::timeout, this, &PcControlImpl::slotHandleExitTimeout);

    // Delay timer
    m_pc_delay_timer.setSingleShot(true);
    connect(&m_pc_delay_timer, &QTimer::timeout, this,
            [this]() { emit signalPcStateChanged(shared::PcState::Normal); });

    // Resolution change events
    connect(&m_message_queue, &MessageQueue::signalResolutionChanged, this,
            [this]()
            {
                qDebug("Resolution change detected!");
                if (m_pending_resolution_change)
                {
                    changeResolution(m_pending_resolution_change->m_width, m_pending_resolution_change->m_height, true);
                }
            });
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void PcControlImpl::launchSteamApp(uint app_id)
{
    m_exit_timer.stop();

    if (app_id == FALLBACK_APP_ID)
    {
        return;
    }

    const bool steam_is_running{isSteamRunning()};
    if (!steam_is_running || getRunningApp() != app_id)
    {
        m_app_reg_key = std::make_unique<RegKey>();
        connect(m_app_reg_key.get(), &RegKey::signalValuesChanged, this,
                [this, app_id](const QMap<QString, QVariant>& changed_values)
                {
                    bool is_running{false};
                    bool is_updating{false};

                    if (changed_values.contains(REG_APP_RUNNING))
                    {
                        is_running =
                            (changed_values[REG_APP_RUNNING].isValid() ? changed_values[REG_APP_RUNNING].toBool()
                                                                       : false);
                        qDebug("App %s \"running\" value change detected: %s", qUtf8Printable(QString::number(app_id)),
                               qUtf8Printable(is_running ? QStringLiteral("true") : QStringLiteral("false")));
                    }
                    if (changed_values.contains(REG_APP_UPDATING))
                    {
                        is_updating =
                            (changed_values[REG_APP_UPDATING].isValid() ? changed_values[REG_APP_UPDATING].toBool()
                                                                        : false);
                        qDebug("App %s \"updating\" value change detected: %s", qUtf8Printable(QString::number(app_id)),
                               qUtf8Printable(is_updating ? QStringLiteral("true") : QStringLiteral("false")));
                    }

                    if (m_launched_app && m_launched_app->m_is_running && !is_running)
                    {
                        // The app was closed, we are no longer watching it
                        m_launched_app = std::nullopt;
                        m_app_reg_key  = nullptr;
                        restoreChangedResolution();
                        return;
                    }

                    m_launched_app = LaunchedAppData{app_id, is_running, is_updating};
                });

        m_app_reg_key->open(REG_STEAM_APPS_PATH + R"(\)" + QString::number(app_id),
                            QStringList{REG_APP_RUNNING, REG_APP_UPDATING});

        QProcess::execute(m_steam_exec, {"-applaunch", QString::number(app_id)});
    }
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::exitSteam(std::optional<uint> grace_period_in_sec)
{
    if (!m_steam_process_tracker.isRunningNow())
    {
        m_exit_timer.stop();
    }

    // Try to shutdown steam gracefully first
    if (!m_steam_exec.isEmpty())
    {
        QProcess::execute(m_steam_exec, {"-shutdown"});
    }

    if (grace_period_in_sec)
    {
        // Restart timer only it is not already running or the grace period has changed
        const auto time_in_msec = grace_period_in_sec.value() * MS_TO_SEC;
        if (!m_exit_timer.isActive() || static_cast<uint>(m_exit_timer.interval()) != time_in_msec)
        {
            m_exit_timer.start(static_cast<int>(time_in_msec));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::shutdownPC(uint grace_period_in_sec)
{
    if (m_pc_delay_timer.isActive())
    {
        qDebug("PC is already being shut down. Aborting request.");
        return;
    }

    QProcess::execute("shutdown", {"-s", "-t", QString::number(grace_period_in_sec), "-f", "-c",
                                   m_app_name + " is putting you to sleep :)", "-y"});

    m_pc_delay_timer.start(static_cast<int>(grace_period_in_sec + EXTRA_DELAY_SECS) * MS_TO_SEC);
    emit signalPcStateChanged(shared::PcState::ShuttingDown);
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::restartPC(uint grace_period_in_sec)
{
    if (m_pc_delay_timer.isActive())
    {
        qDebug("PC is already being restarted. Aborting request.");
        return;
    }

    QProcess::execute("shutdown", {"-r", "-t", QString::number(grace_period_in_sec), "-f", "-c",
                                   m_app_name + " is giving you new live :?", "-y"});

    m_pc_delay_timer.start(static_cast<int>(grace_period_in_sec + EXTRA_DELAY_SECS) * MS_TO_SEC);
    emit signalPcStateChanged(shared::PcState::Restarting);
}

//---------------------------------------------------------------------------------------------------------------------

uint PcControlImpl::getRunningApp() const
{
    return m_launched_app && m_launched_app->m_is_running ? m_launched_app->m_app_id
                                                          : m_global_app_id.value_or(FALLBACK_APP_ID);
}

//---------------------------------------------------------------------------------------------------------------------

std::optional<uint> PcControlImpl::isLastLaunchedAppUpdating() const
{
    return m_launched_app && m_launched_app->m_is_updating ? std::make_optional(m_launched_app->m_app_id)
                                                           : std::nullopt;
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControlImpl::isSteamRunning() const
{
    return m_steam_process_tracker.isRunning();
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::setAutoStart(bool enable)
{
    const auto location{getLinkLocation()};
    if (!location)
    {
        qWarning("Could not determine autostart location!");
        return;
    }

    if (QFile::exists(*location))
    {
        if (!QFile::remove(*location))
        {
            qWarning("Failed to remove %s!", qUtf8Printable(*location));
            return;
        }
    }

    if (enable)
    {
        if (!QFile::link(QCoreApplication::applicationFilePath(), *location))
        {
            qWarning("Failed to create link for %s!", qUtf8Printable(*location));
            return;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControlImpl::isAutoStartEnabled() const
{
    const auto location{getLinkLocation()};
    return location && QFile::exists(*location);
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::slotSteamProcessStateChanged()
{
    if (!m_steam_process_tracker.isRunning())
    {
        m_global_app_id = std::nullopt;
        m_launched_app  = std::nullopt;
        m_app_reg_key   = nullptr;
        restoreChangedResolution();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::slotHandleRegKeyChanges(const QMap<QString, QVariant>& changed_values)
{
    if (changed_values.contains(REG_EXEC))
    {
        m_steam_exec = changed_values[REG_EXEC].isValid() ? changed_values[REG_EXEC].toString() : QString();
        qDebug("Steam exec path: %s", qUtf8Printable(m_steam_exec));
    }
    if (changed_values.contains(REG_APP_ID))
    {
        const auto prev_app_id = m_global_app_id;
        m_global_app_id = changed_values[REG_APP_ID].isValid() ? std::make_optional(changed_values[REG_APP_ID].toUInt())
                                                               : std::nullopt;

        if (prev_app_id != m_global_app_id)
        {
            qDebug("Running appID change detected (via global key): %s",
                   qUtf8Printable(QString::number(m_global_app_id.value_or(FALLBACK_APP_ID))));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::slotHandleExitTimeout()
{
    if (m_steam_process_tracker.isRunningNow())
    {
        m_steam_process_tracker.terminateAll();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::changeResolution(uint width, uint height, bool immediate)
{
    if (immediate)
    {
        qDebug("Changing resolution.");
        m_pending_resolution_change = std::nullopt;
        setResolution(width, height);
    }
    else if (!m_steam_process_tracker.isRunningNow() || getRunningApp() == 0)
    {
        qDebug("Preparing to change resolution after change is detected.");
        m_pending_resolution_change = {width, height};
    }
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::abortPendingResolutionChange()
{
    qDebug("Clearing pending resolution change (if any).");
    m_pending_resolution_change = std::nullopt;
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::restoreChangedResolution()
{
    // Nothing to do here, NVidia does it for us!
    abortPendingResolutionChange();
}
}  // namespace os
