// header file include
#include "os/linux/nativesleepinhibitor.h"

// system/Qt includes
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
NativeSleepInhibitor::NativeSleepInhibitor(const QString& app_name)
{
    QDBusInterface manager_bus{"org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager",
                               QDBusConnection::systemBus()};
    if (!manager_bus.isValid())
    {
        qCWarning(lc::os) << "Could not create QDBusInterface instance for inhibiting sleep!";
        return;
    }

    const auto method{QStringLiteral("Inhibit")};
    const auto what{QStringLiteral("idle:sleep")};
    const auto who{app_name};
    const auto why{QStringLiteral("Gaming Session")};
    const auto mode{QStringLiteral("block")};

    const QDBusReply<QDBusUnixFileDescriptor> reply{manager_bus.call(QDBus::Block, method, what, who, why, mode)};
    if (!reply.isValid())
    {
        qCWarning(lc::os).nospace() << "got invalid reply for " << method << " request: " << reply.error();
        return;
    }

    m_file_descriptor = reply.value();
}
}  // namespace os
