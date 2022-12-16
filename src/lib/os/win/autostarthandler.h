#pragma once

// system/Qt includes
#include <QtGlobal>

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class AutoStartHandler
{
    Q_DISABLE_COPY(AutoStartHandler)

public:
    explicit AutoStartHandler() = default;
    virtual ~AutoStartHandler() = default;

    void setAutoStart(bool enable);
    bool isAutoStartEnabled() const;
};
}  // namespace os
