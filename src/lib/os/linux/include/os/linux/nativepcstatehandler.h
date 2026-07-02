#pragma once

// system/Qt includes
#include <QtDBus/QDBusInterface>

// local includes
#include "os/common/nativepcstatehandlerinterface.h"

namespace os
{
namespace internal
{
class Login1Manager : public QDBusInterface
{
    Q_OBJECT

public:
    explicit Login1Manager();
    ~Login1Manager() override = default;

signals:
    void PrepareForSleep(bool going_to_sleep);
};
}  // namespace internal

class NativePcStateHandler : public NativePcStateHandlerInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(NativePcStateHandler)

public:
    explicit NativePcStateHandler();
    ~NativePcStateHandler() override = default;

    bool canShutdownPC() override;
    bool canRestartPC() override;
    bool canSuspendPC() override;
    bool canHibernatePC() override;

    bool shutdownPC() override;
    bool restartPC() override;
    bool suspendPC() override;
    bool hibernatePC() override;

private:
    internal::Login1Manager m_login1_bus;
};
}  // namespace os
