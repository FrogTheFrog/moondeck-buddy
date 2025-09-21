#pragma once

// system/Qt includes
#include <QtGlobal>

// local includes
#include "os/shared/nativeautostarthandlerinterface.h"

// forward declarations
namespace shared
{
class AppMetadata;
}

namespace os
{
class NativeAutoStartHandler : public NativeAutoStartHandlerInterface
{
    Q_DISABLE_COPY(NativeAutoStartHandler)

public:
    explicit NativeAutoStartHandler(const shared::AppMetadata& app_meta);
    ~NativeAutoStartHandler() override = default;

    void setAutoStart(bool enable) override;
    bool isAutoStartEnabled() const override;

    bool isServiceSupported() const override;
    bool restartIntoService() override;

private:
    const shared::AppMetadata& m_app_meta;
};
}  // namespace os
