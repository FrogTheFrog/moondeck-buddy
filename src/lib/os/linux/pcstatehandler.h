#pragma once

// system/Qt includes
#include <QObject>

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
    explicit PcStateHandler() = default;
    ~PcStateHandler() override = default;

    shared::PcState getState() const override;

    bool shutdownPC(uint grace_period_in_sec) override;
    bool restartPC(uint grace_period_in_sec) override;
    bool suspendPC(uint grace_period_in_sec) override;
};
}  // namespace os
