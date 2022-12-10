// header file include
#include "steamproxy.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
SteamProxy::SteamProxy(PcControlInterface& pc_control)
    : m_pc_control{pc_control}
{
}

//---------------------------------------------------------------------------------------------------------------------

void SteamProxy::slotHandleMessages(const QUuid&                                    socket_id,
                                    const std::variant<msgs::in::LaunchApp, msgs::in::SteamStatus, msgs::in::CloseSteam,
                                                       msgs::in::ChangeResolution>& msg)
{
    if (const auto* const launch_app = std::get_if<msgs::in::LaunchApp>(&msg); launch_app)
    {
        m_pc_control.launchSteamApp(launch_app->m_app_id);
        emit signalSendResponse(socket_id, msgs::out::MessageAccepted{});
        return;
    }

    if (const auto* const steam_status = std::get_if<msgs::in::SteamStatus>(&msg); steam_status)
    {
        emit signalSendResponse(socket_id, msgs::out::SteamStatus{m_pc_control.getRunningApp(),
                                                                  m_pc_control.isLastLaunchedAppUpdating(),
                                                                  m_pc_control.isSteamRunning()});
        return;
    }

    if (const auto* const close_steam = std::get_if<msgs::in::CloseSteam>(&msg); close_steam)
    {
        m_pc_control.exitSteam(close_steam->m_grace_period);
        emit signalSendResponse(socket_id, msgs::out::MessageAccepted{});
        return;
    }

    if (const auto* const change_resolution = std::get_if<msgs::in::ChangeResolution>(&msg); change_resolution)
    {
        m_pc_control.changeResolution(change_resolution->m_width, change_resolution->m_height,
                                      change_resolution->m_immediate);
        emit signalSendResponse(socket_id, msgs::out::MessageAccepted{});
        return;
    }
}

//---------------------------------------------------------------------------------------------------------------------

void SteamProxy::slotHandleConnectivityChange(bool connected)
{
    if (!connected)
    {
        m_pc_control.abortPendingResolutionChange();
    }
}
}  // namespace os
