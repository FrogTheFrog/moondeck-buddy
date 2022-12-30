#pragma once

// system/Qt includes
#include <QtGlobal>

// local includes
#include "../cursorhandlerinterface.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class CursorHandler : public CursorHandlerInterface
{
    Q_DISABLE_COPY(CursorHandler)

public:
    explicit CursorHandler()  = default;
    ~CursorHandler() override = default;

    void hideCursor() override;
};
}  // namespace os
