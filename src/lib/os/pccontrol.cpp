// header file include
#include "os/pccontrol.h"

// system/Qt includes
#include <QFileInfo>

// os-specific includes
#if defined(Q_OS_WIN)
    #include "os/win/nativepcstatehandler.h"
    #include "os/win/nativeprocesshandler.h"
    #include "os/win/nativeresolutionhandler.h"
    #include "os/win/steamregistryobserver.h"
#elif defined(Q_OS_LINUX)
    #include "os/linux/nativepcstatehandler.h"
    #include "os/linux/nativeprocesshandler.h"
    #include "os/linux/nativeresolutionhandler.h"
    #include "os/linux/steamregistryobserver.h"
#else
    #error OS is not supported!
#endif

// local includes
#include "os/pcstatehandler.h"
#include "os/steam/steamprocesstracker.h"
#include "os/streamstatehandler.h"
#include "shared/loggingcategories.h"
#include "utils/appsettings.h"

namespace
{
const int SEC_TO_MS{1000};
}

namespace os
{
PcControl::PcControl(const utils::AppSettings& app_settings)
    : m_app_settings{app_settings}
    , m_auto_start_handler{m_app_settings.getAppMetadata()}
    , m_pc_state_handler{std::make_unique<NativePcStateHandler>()}
    , m_resolution_handler{std::make_unique<NativeResolutionHandler>(), m_app_settings.getHandledDisplays()}
    , m_steam_handler{[this]()
                      {
                          auto exec_path{m_app_settings.getSteamExecutablePath()};
                          if (QFileInfo::exists(exec_path))
                          {
                              return exec_path;
                          }

                          return QString{};
                      },
                      std::make_unique<SteamProcessTracker>(std::make_unique<NativeProcessHandler>()),
                      std::make_unique<SteamRegistryObserver>(m_app_settings.getRegistryFileOverride())}
    , m_stream_state_handler{std::make_unique<StreamStateHandler>(
          m_app_settings.getAppMetadata().getAppName(shared::AppMetadata::App::Stream))}
{
    Q_ASSERT(m_stream_state_handler != nullptr);

    connect(&m_steam_handler, &SteamHandler::signalProcessStateChanged, this,
            &PcControl::slotHandleSteamProcessStateChange);
    connect(m_stream_state_handler.get(), &StreamStateHandler::signalStreamStateChanged, this,
            &PcControl::slotHandleStreamStateChange);
}

// For forward declarations
PcControl::~PcControl() = default;

bool PcControl::launchSteamApp(uint app_id, bool force_big_picture)
{
    return m_steam_handler.launchApp(app_id, force_big_picture);
}

bool PcControl::closeSteam(std::optional<uint> grace_period_in_sec)
{
    return m_steam_handler.close(grace_period_in_sec);
}

bool PcControl::shutdownPC(uint grace_period_in_sec)
{
    if (m_pc_state_handler.shutdownPC(grace_period_in_sec))
    {
        closeSteam(std::nullopt);
        restoreChangedResolution(true);
        emit signalShowTrayMessage(
            "Shutdown in progress", m_app_settings.getAppMetadata().getAppName() + " is putting you to sleep :)",
            QSystemTrayIcon::MessageIcon::Information, static_cast<int>(grace_period_in_sec) * SEC_TO_MS);
        return true;
    }

    return false;
}

bool PcControl::restartPC(uint grace_period_in_sec)
{
    if (m_pc_state_handler.restartPC(grace_period_in_sec))
    {
        closeSteam(std::nullopt);
        restoreChangedResolution(true);
        emit signalShowTrayMessage(
            "Restart in progress", m_app_settings.getAppMetadata().getAppName() + " is giving you new life :?",
            QSystemTrayIcon::MessageIcon::Information, static_cast<int>(grace_period_in_sec) * SEC_TO_MS);
        return true;
    }

    return false;
}

bool PcControl::suspendPC(uint grace_period_in_sec, bool close_steam)
{
    if (m_pc_state_handler.suspendPC(grace_period_in_sec))
    {
        if (close_steam)
        {
            closeSteam(std::nullopt);
            restoreChangedResolution(true);
        }
        else
        {
            endStream();
        }

        emit signalShowTrayMessage(
            "Suspend in progress",
            m_app_settings.getAppMetadata().getAppName() + " is about to suspend you real hard :P",
            QSystemTrayIcon::MessageIcon::Information, static_cast<int>(grace_period_in_sec) * SEC_TO_MS);
        return true;
    }

    return false;
}

bool PcControl::hibernatePC(uint grace_period_in_sec, bool close_steam)
{
    if (m_pc_state_handler.hibernatePC(grace_period_in_sec))
    {
        if (close_steam)
        {
            closeSteam(std::nullopt);
            restoreChangedResolution(true);
        }
        else
        {
            endStream();
        }

        emit signalShowTrayMessage(
            "Hibernation in progress",
            m_app_settings.getAppMetadata().getAppName() + " is about to put you into hard sleep :O",
            QSystemTrayIcon::MessageIcon::Information, static_cast<int>(grace_period_in_sec) * SEC_TO_MS);
        return true;
    }

    return false;
}

bool PcControl::endStream()
{
    return m_stream_state_handler->endStream();
}

uint PcControl::getRunningApp() const
{
    return m_steam_handler.getRunningApp();
}

std::optional<uint> PcControl::getTrackedUpdatingApp() const
{
    return m_steam_handler.getTrackedUpdatingApp();
}

bool PcControl::isSteamRunning() const
{
    return m_steam_handler.isRunning();
}

enums::StreamState PcControl::getStreamState() const
{
    return m_stream_state_handler->getCurrentState();
}

enums::PcState PcControl::getPcState() const
{
    return m_pc_state_handler.getState();
}

void PcControl::setAutoStart(bool enable)
{
    m_auto_start_handler.setAutoStart(enable);
}

bool PcControl::isAutoStartEnabled() const
{
    return m_auto_start_handler.isAutoStartEnabled();
}

bool PcControl::changeResolution(uint width, uint height)
{
    return m_resolution_handler.changeResolution(width, height);
}

void PcControl::restoreChangedResolution(bool force)
{
    const bool not_streaming{m_stream_state_handler->getCurrentState() == enums::StreamState::NotStreaming};
    const bool tracked_app_is_dead{m_steam_handler.getTrackedActiveApp() == std::nullopt};

    if ((not_streaming && tracked_app_is_dead) || force)
    {
        m_resolution_handler.restoreResolution();
    }
}

void PcControl::slotHandleSteamProcessStateChange()
{
    if (m_steam_handler.isRunning())
    {
        qCDebug(lc::os) << "Handling Steam start.";
    }
    else
    {
        qCDebug(lc::os) << "Handling Steam exit.";
        endStream();
        restoreChangedResolution(false);
    }
}

void PcControl::slotHandleStreamStateChange()
{
    switch (m_stream_state_handler->getCurrentState())
    {
        case enums::StreamState::NotStreaming:
        {
            qCDebug(lc::os) << "Stream has ended.";
            restoreChangedResolution(false);
            break;
        }
        case enums::StreamState::Streaming:
        {
            qCDebug(lc::os) << "Stream started.";
            break;
        }
        case enums::StreamState::StreamEnding:
        {
            qCDebug(lc::os) << "Stream is ending.";
            break;
        }
    }
}

void PcControl::slotAppTrackingHasEnded()
{
    restoreChangedResolution(false);
}
}  // namespace os
