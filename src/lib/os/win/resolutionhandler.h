#pragma once

// A SEPARATE WINDOWS INCLUDE BECAUSE OF THE SMELL!
#include <windows.h>

// system/Qt includes
#include <QObject>

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class ResolutionHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ResolutionHandler)

public:
    explicit ResolutionHandler() = default;
    ~ResolutionHandler() override;

    bool changeResolution(uint width, uint height);
    void restoreResolution();

    void setPendingResolution(uint width, uint height);
    void applyPendingChange();
    void clearPendingResolution();

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
