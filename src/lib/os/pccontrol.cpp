// header file include
#include "pccontrol.h"

// os-specific includes
#if defined(Q_OS_WIN)
    #include "win/autostarthandler.h"
    #include "win/cursorhandler.h"
    #include "win/pcstatehandler.h"
    #include "win/resolutionhandler.h"
    #include "win/steamhandler.h"
#elif defined(Q_OS_LINUX)
    #include "linux/autostarthandler.h"
    #include "linux/cursorhandler.h"
    #include "linux/pcstatehandler.h"
    #include "linux/resolutionhandler.h"
    #include "linux/steamhandler.h"
#else
    #error OS is not supported!
#endif

// local includes
#include "shared/constants.h"
#include "shared/loggingcategories.h"
#include "shared/streamstatehandler.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const int SEC_TO_MS{1000};
}

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
PcControl::PcControl()
    : m_auto_start_handler{std::make_unique<AutoStartHandler>()}
    , m_cursor_handler{std::make_unique<CursorHandler>()}
    , m_pc_state_handler{std::make_unique<PcStateHandler>()}
    , m_resolution_handler{std::make_unique<ResolutionHandler>()}
    , m_steam_handler{std::make_unique<SteamHandler>()}
    , m_stream_state_handler{std::make_unique<StreamStateHandler>()}
{
    connect(m_steam_handler.get(), &SteamHandler::signalSteamStateChanged, this,
            &PcControl::slotHandleSteamStateChange);
    connect(m_stream_state_handler.get(), &StreamStateHandler::signalStreamStateChanged, this,
            &PcControl::slotHandleStreamStateChange);
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControl::launchSteamApp(uint app_id)
{
    QStringList steam_args;
    if (m_stream_state_handler->getCurrentState() == shared::StreamState::Streaming || !m_steam_handler->isRunningNow())
    {
        steam_args += "-bigpicture";
    }
    return m_steam_handler->launchApp(app_id, steam_args);
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControl::closeSteam(std::optional<uint> grace_period_in_sec)
{
    return m_steam_handler->close(grace_period_in_sec);
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControl::shutdownPC(uint grace_period_in_sec)
{
    if (m_pc_state_handler->shutdownPC(grace_period_in_sec))
    {
        closeSteam(std::nullopt);
        emit signalShowTrayMessage("Shutdown in progress", shared::APP_NAME_BUDDY + " is putting you to sleep :)",
                                   QSystemTrayIcon::MessageIcon::Information,
                                   static_cast<int>(grace_period_in_sec) * SEC_TO_MS);
        return true;
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControl::restartPC(uint grace_period_in_sec)
{
    if (m_pc_state_handler->restartPC(grace_period_in_sec))
    {
        closeSteam(std::nullopt);
        emit signalShowTrayMessage("Restart in progress", shared::APP_NAME_BUDDY + " is giving you new life :?",
                                   QSystemTrayIcon::MessageIcon::Information,
                                   static_cast<int>(grace_period_in_sec) * SEC_TO_MS);
        return true;
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControl::suspendPC(uint grace_period_in_sec)
{
    if (m_pc_state_handler->suspendPC(grace_period_in_sec))
    {
        closeSteam(std::nullopt);
        emit signalShowTrayMessage(
            "Suspend in progress", shared::APP_NAME_BUDDY + " is about to suspend you real hard :P",
            QSystemTrayIcon::MessageIcon::Information, static_cast<int>(grace_period_in_sec) * SEC_TO_MS);
        return true;
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------

uint PcControl::getRunningApp() const
{
    return m_steam_handler->getRunningApp();
}

//---------------------------------------------------------------------------------------------------------------------

std::optional<uint> PcControl::getTrackedUpdatingApp() const
{
    return m_steam_handler->getTrackedUpdatingApp();
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControl::isSteamRunning() const
{
    return m_steam_handler->isRunning();
}

//---------------------------------------------------------------------------------------------------------------------

shared::StreamState PcControl::getStreamState() const
{
    return m_stream_state_handler->getCurrentState();
}

//---------------------------------------------------------------------------------------------------------------------

shared::PcState PcControl::getPcState() const
{
    return m_pc_state_handler->getState();
}

//---------------------------------------------------------------------------------------------------------------------

void PcControl::setAutoStart(bool enable)
{
    m_auto_start_handler->setAutoStart(enable);
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControl::isAutoStartEnabled() const
{
    return m_auto_start_handler->isAutoStartEnabled();
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControl::changeResolution(uint width, uint height, bool immediate)
{
    if (immediate)
    {
        return m_resolution_handler->changeResolution(width, height);
    }

    if (!m_steam_handler->isRunningNow() || getRunningApp() == 0)
    {
        m_resolution_handler->setPendingResolution(width, height);
        return true;
    }

    qCDebug(lc::os) << "Non-immediate change is discarded.";
    return false;
}

//---------------------------------------------------------------------------------------------------------------------

void PcControl::abortPendingResolutionChange()
{
    m_resolution_handler->clearPendingResolution();
}

//---------------------------------------------------------------------------------------------------------------------

void PcControl::restoreChangedResolution()
{
    m_resolution_handler->restoreResolution();
}

//---------------------------------------------------------------------------------------------------------------------

void PcControl::slotHandleSteamStateChange()
{
    if (m_steam_handler->isRunning())
    {
        qCDebug(lc::os) << "Handling Steam start.";
        m_resolution_handler->clearPendingResolution();
    }
    else
    {
        qCDebug(lc::os) << "Handling Steam exit.";
        m_stream_state_handler->endStream();
        m_resolution_handler->restoreResolution();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void PcControl::slotHandleStreamStateChange()
{
    switch (m_stream_state_handler->getCurrentState())
    {
        case shared::StreamState::NotStreaming:
        {
            qCDebug(lc::os) << "Stream has ended.";
            break;
        }
        case shared::StreamState::Streaming:
        {
            qCDebug(lc::os) << "Stream started.";
            m_resolution_handler->applyPendingChange();
            m_cursor_handler->hideCursor();
            break;
        }
        case shared::StreamState::StreamEnding:
        {
            qCDebug(lc::os) << "Stream is ending.";
            m_steam_handler->close(std::nullopt);
            m_resolution_handler->restoreResolution();
            break;
        }
    }
}
}  // namespace os
