#pragma once

// local includes
#include "msgs/in/closesteam.h"
#include "msgs/in/launchapp.h"
#include "msgs/in/steamstatus.h"
#include "msgs/out/messageaccepted.h"
#include "msgs/out/steamstatus.h"
#include "pccontrolinterface.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class SteamProxy : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SteamProxy)

public:
    explicit SteamProxy(PcControlInterface& pc_control);
    ~SteamProxy() override = default;

signals:
    void signalSendResponse(const QUuid&                                                            socket_id,
                            const std::variant<msgs::out::SteamStatus, msgs::out::MessageAccepted>& msg);

public slots:
    void slotHandleMessages(const QUuid& socket_id,
                            const std::variant<msgs::in::LaunchApp, msgs::in::SteamStatus, msgs::in::CloseSteam>& msg);

private:
    PcControlInterface& m_pc_control;
};
}  // namespace os
