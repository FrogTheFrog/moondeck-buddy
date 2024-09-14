#pragma once

// system/Qt includes
#include <memory>

// forward declarations
namespace shared
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
    explicit AutoStartHandler(const shared::AppMetadata& app_meta);
    ~AutoStartHandler();

    void setAutoStart(bool enable);
    bool isAutoStartEnabled() const;

private:
    std::unique_ptr<NativeAutoStartHandlerInterface> m_impl;
};
}  // namespace os
