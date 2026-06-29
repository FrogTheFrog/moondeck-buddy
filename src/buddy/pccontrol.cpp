// header file include
#include "pccontrol.h"

// local includes
#include "common/appsettings.h"
#include "common/loggingcategories.h"
#include "steam/steamprocesstracker.h"
#include "streamstatehandler.h"

PcControl::PcControl(const common::AppSettings& app_settings)
    : m_app_settings{app_settings}
    , m_auto_start_handler{m_app_settings.m_app_metadata}
    , m_steam_handler{m_app_settings}
    , m_stream_state_handler{m_app_settings.m_app_metadata.getAppName(common::AppMetadata::App::Stream)}
    , m_shared_env_reader{m_app_settings.m_app_metadata.getSharedEnvMapKey()}
{
    connect(&m_steam_handler, &steam::SteamHandler::signalSteamClosed, this, &PcControl::slotHandleSteamClosed);
    connect(&m_steam_handler, &steam::SteamHandler::signalTrackedAppDataChanged, this,
            &PcControl::signalTrackedAppDataChanged);
    connect(&m_steam_handler, &steam::SteamHandler::signalSteamUiModeChanged, this,
            &PcControl::signalSteamUiModeChanged);
    connect(&m_steam_handler, &steam::SteamHandler::signalSteamCurrentUserChanged, this,
            &PcControl::signalSteamCurrentUserChanged);
    connect(&m_stream_state_handler, &StreamStateHandler::signalStreamStateChanged, this,
            &PcControl::slotHandleStreamStateChange);
    connect(&m_pc_state_handler, &os::PcStateHandler::signalShowTrayMessage, this, &PcControl::signalShowTrayMessage);
}

// For forward declarations
PcControl::~PcControl() = default;

bool PcControl::launchSteam(const bool big_picture_mode, const QString& username)
{
    return m_steam_handler.launchSteam(big_picture_mode, username, m_cached_env);
}

enums::SteamUiMode PcControl::getSteamUiMode() const
{
    return m_steam_handler.getSteamUiMode();
}

bool PcControl::closeSteam(const bool keep_stream_alive)
{
    const auto return_value{m_steam_handler.close()};
    if (return_value)
    {
        m_keep_stream_alive = keep_stream_alive;
    }

    return return_value;
}

bool PcControl::closeSteamBigPictureMode()
{
    return m_steam_handler.closeBigPictureMode();
}

bool PcControl::launchSteamApp(const steam::AppId& app_id)
{
    return m_steam_handler.launchApp(app_id, m_cached_env);
}

std::optional<std::tuple<steam::AppId, enums::AppState>>
    PcControl::getAppData(const std::optional<steam::AppId>& app_id) const
{
    return m_steam_handler.getAppData(app_id);
}

bool PcControl::clearAppData()
{
    m_steam_handler.clearSessionData();
    return true;
}

std::optional<std::map<steam::AppId, QString>> PcControl::getNonSteamAppData(const steam::SteamId& user_id) const
{
    return m_steam_handler.getNonSteamAppData(user_id);
}

std::optional<steam::SteamId> PcControl::getCurrentUserId() const
{
    return m_steam_handler.getCurrentUserId();
}

bool PcControl::shutdownPC(const uint delay_in_seconds)
{
    return m_pc_state_handler.shutdownPC(delay_in_seconds);
}

bool PcControl::restartPC(const uint delay_in_seconds)
{
    return m_pc_state_handler.restartPC(delay_in_seconds);
}

bool PcControl::suspendPC(const uint delay_in_seconds)
{
    return m_pc_state_handler.suspendPC(delay_in_seconds);
}

bool PcControl::hibernatePC(const uint delay_in_seconds)
{
    return m_pc_state_handler.hibernatePC(delay_in_seconds);
}

bool PcControl::abortPcStateChange()
{
    return m_pc_state_handler.abortPcStateChange();
}

bool PcControl::endStream()
{
    return m_stream_state_handler.endStream();
}

enums::StreamState PcControl::getStreamState() const
{
    return m_stream_state_handler.getCurrentState();
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

bool PcControl::isServiceSupported() const
{
    return m_auto_start_handler.isServiceSupported();
}

bool PcControl::restartIntoService()
{
    return m_auto_start_handler.restartIntoService();
}

void PcControl::slotHandleSteamClosed()
{
    if (!m_keep_stream_alive)
    {
        endStream();
    }
}

void PcControl::slotHandleStreamStateChange()
{
    switch (m_stream_state_handler.getCurrentState())
    {
        case enums::StreamState::NotStreaming:
        {
            qCInfo(lc::buddyMain) << "Stream has ended.";
            break;
        }
        case enums::StreamState::Streaming:
        {
            qCInfo(lc::buddyMain) << "Stream started.";
            if (auto env = m_shared_env_reader.read<decltype(m_cached_env)>())
            {
                m_cached_env = std::move(*env);
                if (!m_cached_env.empty())
                {
                    qCInfo(lc::buddyMain) << "Got the following ENV from Stream:";
                    for (const auto& [key, value] : m_cached_env.asKeyValueRange())
                    {
                        qCInfo(lc::buddyMain) << "  " << key << "=" << value;
                    }
                }
            }
            break;
        }
        case enums::StreamState::StreamEnding:
        {
            qCInfo(lc::buddyMain) << "Stream is ending.";
            m_steam_handler.clearSessionData();
            m_cached_env.clear();
            break;
        }
    }

    emit signalStreamStateChanged();
}
