#pragma once

// system/Qt includes
#include <QObject>
#include <QTimer>

// local includes
#include "shared/enums.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class PcStateHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(PcStateHandler)

public:
    explicit PcStateHandler();
    ~PcStateHandler() override = default;

    shared::PcState getState() const;

    bool shutdownPC(uint grace_period_in_sec);
    bool restartPC(uint grace_period_in_sec);

private:
    QString         m_app_name;
    QTimer          m_state_change_back_timer;
    shared::PcState m_state{shared::PcState::Normal};
};
}  // namespace os
