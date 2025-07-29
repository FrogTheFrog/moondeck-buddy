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

bool PcControl::launchSteam(const bool big_picture_mode)
{
    return m_steam_handler.launchSteam(big_picture_mode);
}

enums::SteamUiMode PcControl::getSteamUiMode() const
{
    return m_steam_handler.getSteamUiMode();
}

bool PcControl::closeSteam()
{
    return m_steam_handler.close();
}

bool PcControl::closeSteamBigPictureMode()
{
    return m_steam_handler.closeBigPictureMode();
}

bool PcControl::launchSteamApp(std::uint64_t app_id)
{
    return m_steam_handler.launchApp(app_id);
}

std::optional<std::tuple<std::uint64_t, enums::AppState>>
    PcControl::getAppData(const std::optional<std::uint64_t>& app_id) const
{
    return m_steam_handler.getAppData(app_id);
}

bool PcControl::clearAppData()
{
    m_steam_handler.clearSessionData();
    return true;
}

std::optional<std::map<std::uint64_t, QString>> PcControl::getNonSteamAppData(const std::uint64_t user_id) const
{
    return m_steam_handler.getNonSteamAppData(user_id);
}

bool PcControl::shutdownPC(const uint delay_in_seconds)
{
    if (m_pc_state_handler.shutdownPC(delay_in_seconds))
    {
        closeSteam();
        endStream();
        emit signalShowTrayMessage("Shutdown in progress",
                                   m_app_settings.getAppMetadata().getAppName() + " is putting you to sleep :)",
                                   QSystemTrayIcon::MessageIcon::Information, delay_in_seconds * 1000);
        return true;
    }

    return false;
}

bool PcControl::restartPC(const uint delay_in_seconds)
{
    if (m_pc_state_handler.restartPC(delay_in_seconds))
    {
        closeSteam();
        endStream();
        emit signalShowTrayMessage("Restart in progress",
                                   m_app_settings.getAppMetadata().getAppName() + " is giving you new life :?",
                                   QSystemTrayIcon::MessageIcon::Information, delay_in_seconds * 1000);
        return true;
    }

    return false;
}

bool PcControl::suspendOrHibernatePC(const uint delay_in_seconds)
{
    const bool hibernation{m_app_settings.getPreferHibernation()};
    const bool result{hibernation ? m_pc_state_handler.hibernatePC(delay_in_seconds)
                                  : m_pc_state_handler.suspendPC(delay_in_seconds)};
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
            QSystemTrayIcon::MessageIcon::Information, delay_in_seconds * 1000);
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
