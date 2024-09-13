#pragma once

// system/Qt includes
#include <QtDBus/QDBusInterface>

// local includes
#include "os/shared/nativepcstatehandlerinterface.h"

namespace os
{
class NativePcStateHandler
    : public QObject
    , public NativePcStateHandlerInterface
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
    QDBusInterface m_logind_bus;
};
}  // namespace os
