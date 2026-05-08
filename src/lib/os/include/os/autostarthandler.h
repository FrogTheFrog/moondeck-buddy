#pragma once

// system/Qt includes
#include <memory>

// forward declarations
namespace common
{
class AppMetadata;
}
namespace os
{
class NativeAutoStartHandlerInterface;
}

namespace os
{
class AutoStartHandler final
{
public:
    explicit AutoStartHandler(const common::AppMetadata& app_meta);
    ~AutoStartHandler();

    void setAutoStart(bool enable);
    bool isAutoStartEnabled() const;

    bool isServiceSupported() const;
    bool restartIntoService();

private:
    std::unique_ptr<NativeAutoStartHandlerInterface> m_impl;
};
}  // namespace os
