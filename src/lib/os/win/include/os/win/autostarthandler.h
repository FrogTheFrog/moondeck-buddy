#pragma once

// system/Qt includes
#include <QtGlobal>

// local includes
#include "os/shared/autostarthandlerinterface.h"

// forward declarations
namespace shared
{
class AppMetadata;
}

namespace os
{
class AutoStartHandler : public AutoStartHandlerInterface
{
    Q_DISABLE_COPY(AutoStartHandler)

public:
    explicit AutoStartHandler(const shared::AppMetadata& app_meta);
    virtual ~AutoStartHandler() override = default;

    void setAutoStart(bool enable) override;
    bool isAutoStartEnabled() const override;

private:
    const shared::AppMetadata& m_app_meta;
};
}  // namespace os
