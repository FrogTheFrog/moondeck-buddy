#pragma once

// system/Qt includes
#include <QTimer>
#include <memory>
#include <set>

// local includes
#include "nativeresolutionhandlerinterface.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class ResolutionHandler: public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ResolutionHandler)

public:
    using Resolution = NativeResolutionHandlerInterface::Resolution;

    explicit ResolutionHandler(std::unique_ptr<NativeResolutionHandlerInterface> native_handler,
                               std::set<QString>                                 handled_displays);
    virtual ~ResolutionHandler();

    bool changeResolution(uint width, uint height);
    void restoreResolution();

private:
    std::unique_ptr<NativeResolutionHandlerInterface> m_native_handler;
    std::set<QString>                                 m_handled_displays;
    std::map<QString, Resolution>                     m_original_resolutions;
    QTimer                                            m_restore_retry_timer;
};
}  // namespace os
