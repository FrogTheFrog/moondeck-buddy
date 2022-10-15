// header file include
#include "commandproxy.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
CommandProxy::CommandProxy(PcControlInterface& pc_control)
    : m_pc_control{pc_control}
{
    connect(&m_pc_control, &PcControlInterface::signalPcStateChanged, this, &CommandProxy::signalPcStateChanged);
}

//---------------------------------------------------------------------------------------------------------------------

void CommandProxy::slotHandleMessages(const std::variant<msgs::in::RestartPc, msgs::in::ShutdownPc>& msg)
{
    if (const auto* const restart = std::get_if<msgs::in::RestartPc>(&msg); restart)
    {
        qDebug("About to restart the PC");
        m_pc_control.exitSteam(std::nullopt);
        m_pc_control.restartPC(restart->m_grace_period);
        return;
    }

    if (const auto* const shutdown = std::get_if<msgs::in::ShutdownPc>(&msg); shutdown)
    {
        qDebug("About to shutdown the PC");
        m_pc_control.exitSteam(std::nullopt);
        m_pc_control.shutdownPC(shutdown->m_grace_period);
        return;
    }
}
}  // namespace os
