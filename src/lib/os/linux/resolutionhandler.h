#pragma once

// system/Qt includes
#include <QObject>

// local includes
#include "../resolutionhandlerinterface.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class ResolutionHandler
    : public QObject
    , public ResolutionHandlerInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(ResolutionHandler)

public:
    explicit ResolutionHandler() = default;
    ~ResolutionHandler() override;

    bool changeResolution(uint width, uint height) override;
    void restoreResolution() override;

    void setPendingResolution(uint width, uint height) override;
    void applyPendingChange() override;
    void clearPendingResolution() override;
};
}  // namespace os
