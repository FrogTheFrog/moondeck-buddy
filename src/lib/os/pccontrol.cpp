// header file include
#include "os/pccontrol.h"

// os-specific includes
#if defined(Q_OS_WIN)
    #include "os/win/nativepcstatehandler.h"
    #include "os/win/nativeprocesshandler.h"
#elif defined(Q_OS_LINUX)
    #include "os/linux/nativepcstatehandler.h"
    #include "os/linux/nativeprocesshandler.h"
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
constexpr int DEFAULT_TIMEOUT_S{10};
constexpr int DEFAULT_TIMEOUT_MS{DEFAULT_TIMEOUT_S * 1000};
}  // namespace

namespace os
{
PcControl::PcControl(const utils::AppSettings& app_settings)
    : m_app_settings{app_settings}
    , m_auto_start_handler{m_app_settings.getAppMetadata()}
    , m_pc_state_handler{std::make_unique<NativePcStateHandler>()}
    , m_steam_handler{m_app_settings, std::make_unique<NativeProcessHandler>()}
    , m_stream_state_handler{std::make_unique<StreamStateHandler>(
          m_app_settings.getAppMetadata().getAppName(shared::AppMetadata::App::Stream))}
{
    Q_ASSERT(m_stream_state_handler != nullptr);

    connect(&m_steam_handler, &SteamHandler::signalSteamClosed, this, &PcControl::slotHandleSteamClosed);
    connect(m_stream_state_handler.get(), &StreamStateHandler::signalStreamStateChanged, this,
            &PcControl::slotHandleStreamStateChange);
}

// For forward declarations
PcControl::~PcControl() = default;

bool PcControl::isSteamReady() const
{
    return m_steam_handler.isSteamReady();
}

bool PcControl::closeSteam()
{
    return m_steam_handler.close();
}

bool PcControl::launchSteamApp(uint app_id)
{
    return m_steam_handler.launchApp(app_id);
}

std::optional<std::tuple<uint, enums::AppState>> PcControl::getAppData() const
{
    return m_steam_handler.getAppData();
}

bool PcControl::shutdownPC()
{
    if (m_pc_state_handler.shutdownPC(DEFAULT_TIMEOUT_S))
    {
        closeSteam();
        endStream();
        emit signalShowTrayMessage("Shutdown in progress",
                                   m_app_settings.getAppMetadata().getAppName() + " is putting you to sleep :)",
                                   QSystemTrayIcon::MessageIcon::Information, DEFAULT_TIMEOUT_MS);
        return true;
    }

    return false;
}

bool PcControl::restartPC()
{
    if (m_pc_state_handler.restartPC(DEFAULT_TIMEOUT_S))
    {
        closeSteam();
        endStream();
        emit signalShowTrayMessage("Restart in progress",
                                   m_app_settings.getAppMetadata().getAppName() + " is giving you new life :?",
                                   QSystemTrayIcon::MessageIcon::Information, DEFAULT_TIMEOUT_MS);
        return true;
    }

    return false;
}

bool PcControl::suspendOrHibernatePC()
{
    const bool hibernation{m_app_settings.getPreferHibernation()};
    const bool result{hibernation ? m_pc_state_handler.hibernatePC(DEFAULT_TIMEOUT_S)
                                  : m_pc_state_handler.suspendPC(DEFAULT_TIMEOUT_S)};
    if (result)
    {
        if (m_app_settings.getCloseSteamBeforeSleep())
        {
            closeSteam();
        }
        endStream();

        emit signalShowTrayMessage(
            hibernation ? "Hibernation in progress" : "Suspend in progress",
            m_app_settings.getAppMetadata().getAppName()
                + (hibernation ? " is about to put you into hard sleep :O" : " is about to suspend you real hard :P"),
            QSystemTrayIcon::MessageIcon::Information, DEFAULT_TIMEOUT_MS);
        return true;
    }

    return false;
}

bool PcControl::endStream()
{
    return m_stream_state_handler->endStream();
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

void PcControl::slotHandleSteamClosed()
{
    endStream();
}

void PcControl::slotHandleStreamStateChange()
{
    switch (m_stream_state_handler->getCurrentState())
    {
        case enums::StreamState::NotStreaming:
        {
            qCInfo(lc::os) << "Stream has ended.";
            break;
        }
        case enums::StreamState::Streaming:
        {
            qCInfo(lc::os) << "Stream started.";
            break;
        }
        case enums::StreamState::StreamEnding:
        {
            qCInfo(lc::os) << "Stream is ending.";
            m_steam_handler.clearSessionData();
            break;
        }
    }
}
}  // namespace os
