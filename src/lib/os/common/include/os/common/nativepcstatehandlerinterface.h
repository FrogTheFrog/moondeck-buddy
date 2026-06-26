#pragma once

// system/Qt includes
#include <QObject>

namespace os
{
class NativePcStateHandlerInterface : public QObject
{
    Q_OBJECT

public:
    ~NativePcStateHandlerInterface() override = default;

    virtual bool canShutdownPC()  = 0;
    virtual bool canRestartPC()   = 0;
    virtual bool canSuspendPC()   = 0;
    virtual bool canHibernatePC() = 0;

    virtual bool shutdownPC()  = 0;
    virtual bool restartPC()   = 0;
    virtual bool suspendPC()   = 0;
    virtual bool hibernatePC() = 0;

signals:
    void signalWokeUp();
};
}  // namespace os
