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

    void shutdownPC(uint grace_period_in_sec);
    void restartPC(uint grace_period_in_sec);

signals:
    void signalPcStateChanged(shared::PcState state);

private:
    QString m_app_name;
    QTimer  m_state_change_back_timer;
};
}  // namespace os
