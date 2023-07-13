#pragma once

// system/Qt includes
#include <QObject>

// local includes
#include "../nativepcstatehandlerinterface.h"

//---------------------------------------------------------------------------------------------------------------------

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
    bool m_privilege_acquired;
};
}  // namespace os
