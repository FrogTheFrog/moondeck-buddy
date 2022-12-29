#pragma once

// A SEPARATE WINDOWS INCLUDE BECAUSE OF THE SMELL!
#include <windows.h>

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

private:
    struct Resolution
    {
        uint m_width;
        uint m_height;
    };

    std::optional<Resolution>     m_pending_change;
    std::map<QString, Resolution> m_original_resolutions;
};
}  // namespace os
