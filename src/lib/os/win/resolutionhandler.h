#pragma once

// system/Qt includes
#include <QObject>

// local includes
#include "messagequeue.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class ResolutionHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ResolutionHandler)

public:
    explicit ResolutionHandler();
    ~ResolutionHandler() override;

    void changeResolution(uint width, uint height);
    void restoreResolution();

    void setPendingResolution(uint width, uint height);
    void applyPendingChange();
    void clearPendingResolution();

private slots:
    void slotHandleDetectedChange();

private:
    struct Resolution
    {
        uint m_width;
        uint m_height;
    };

    MessageQueue                  m_message_queue;
    std::optional<Resolution>     m_pending_change;
    std::map<QString, Resolution> m_original_resolutions;
};
}  // namespace os
