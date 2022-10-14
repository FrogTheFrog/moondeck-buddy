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

void SteamProxy::slotHandleMessages(
    const QUuid& socket_id, const std::variant<msgs::in::LaunchApp, msgs::in::SteamStatus, msgs::in::CloseSteam>& msg)
{
    if (const auto* const launch_app = std::get_if<msgs::in::LaunchApp>(&msg); launch_app)
    {
        m_pc_control.launchSteamApp(launch_app->m_app_id);
        emit signalSendResponse(socket_id, msgs::out::MessageAccepted{});
        return;
    }

    if (const auto* const steam_status = std::get_if<msgs::in::SteamStatus>(&msg); steam_status)
    {
        emit signalSendResponse(socket_id,
                                msgs::out::SteamStatus{m_pc_control.getRunningApp(), m_pc_control.isSteamRunning()});
        return;
    }

    if (const auto* const close_steam = std::get_if<msgs::in::CloseSteam>(&msg); close_steam)
    {
        m_pc_control.exitSteam(close_steam->m_grace_period);
        emit signalSendResponse(socket_id, msgs::out::MessageAccepted{});
        return;
    }
}
}  // namespace os
