#pragma once

// system/Qt includes
#include <QObject>
#include <QTimer>

// local includes
#include "../pcstatehandlerinterface.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class PcStateHandler
    : public QObject
    , public PcStateHandlerInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(PcStateHandler)

public:
    explicit PcStateHandler();
    ~PcStateHandler() override = default;

    shared::PcState getState() const override;

    bool shutdownPC(uint grace_period_in_sec) override;
    bool restartPC(uint grace_period_in_sec) override;
    bool suspendPC(uint grace_period_in_sec) override;

public slots:
    void slotResetState();

private:
    QString         m_app_name;
    QTimer          m_state_change_back_timer;
    bool            m_privilege_acquired;
    shared::PcState m_state{shared::PcState::Normal};
};
}  // namespace os
