#pragma once

// system/Qt includes
#include <QtGlobal>

namespace os
{
struct TrackedAppData
{
    uint m_app_id;
    bool m_is_running;
    bool m_is_updating;
};
}  // namespace os