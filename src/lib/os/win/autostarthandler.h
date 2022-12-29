#pragma once

// system/Qt includes
#include <QtGlobal>

// local includes
#include "../autostarthandlerinterface.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class AutoStartHandler : public AutoStartHandlerInterface
{
    Q_DISABLE_COPY(AutoStartHandler)

public:
    explicit AutoStartHandler()          = default;
    virtual ~AutoStartHandler() override = default;

    void setAutoStart(bool enable) override;
    bool isAutoStartEnabled() const override;
};
}  // namespace os
