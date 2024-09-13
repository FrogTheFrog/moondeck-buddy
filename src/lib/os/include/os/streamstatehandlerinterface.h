#pragma once

// system/Qt includes
#include <QObject>

// local includes
#include "shared/enums.h"

namespace os
{
class StreamStateHandlerInterface : public QObject
{
    Q_OBJECT

public:
    ~StreamStateHandlerInterface() override = default;

    virtual bool               endStream()             = 0;
    virtual enums::StreamState getCurrentState() const = 0;

signals:
    void signalStreamStateChanged();
};
}  // namespace os
