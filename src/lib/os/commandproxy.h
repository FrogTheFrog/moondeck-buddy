#pragma once

// local includes
#include "msgs/in/closesteam.h"
#include "msgs/in/restartpc.h"
#include "msgs/in/shutdownpc.h"
#include "pccontrolinterface.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class CommandProxy : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(CommandProxy)

public:
    explicit CommandProxy(PcControlInterface& pc_control);
    ~CommandProxy() override = default;

    // NOLINTNEXTLINE(readability-redundant-access-specifiers)
public slots:
    void slotHandleMessages(const std::variant<msgs::in::RestartPc, msgs::in::ShutdownPc, msgs::in::CloseSteam>& msg);

signals:
    void signalPcStateChanged(shared::PcState state);

private:
    PcControlInterface& m_pc_control;
};
}  // namespace os
