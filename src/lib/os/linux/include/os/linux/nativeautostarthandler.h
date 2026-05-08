#pragma once

// system/Qt includes
#include <QtGlobal>

// local includes
#include "os/common/nativeautostarthandlerinterface.h"

// forward declarations
namespace common
{
class AppMetadata;
}

namespace os
{
class NativeAutoStartHandler : public NativeAutoStartHandlerInterface
{
    Q_DISABLE_COPY(NativeAutoStartHandler)

public:
    explicit NativeAutoStartHandler(const common::AppMetadata& app_meta);
    ~NativeAutoStartHandler() override = default;

    void setAutoStart(bool enable) override;
    bool isAutoStartEnabled() const override;

    bool isServiceSupported() const override;
    bool restartIntoService() override;

private:
    const common::AppMetadata& m_app_meta;
};
}  // namespace os
