#pragma once

// system/Qt includes
#include <QString>
#include <memory>

// forward declarations
namespace os
{
class NativeSleepInhibitorInterface;
}

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class SleepInhibitor final
{
public:
    explicit SleepInhibitor(const QString& app_name);
    ~SleepInhibitor();

private:
    std::unique_ptr<NativeSleepInhibitorInterface> m_impl;
};
}  // namespace os
