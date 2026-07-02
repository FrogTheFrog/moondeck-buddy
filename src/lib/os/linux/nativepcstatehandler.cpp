// header file include
#include "os/linux/nativepcstatehandler.h"

// system/Qt includes
#include <QtDBus/QDBusReply>

// local includes
#include "common/loggingcategories.h"

namespace
{
// NOLINTNEXTLINE(*-swappable-parameters)
bool canDoQuery(QDBusInterface& bus, const QString& log_entry, const QString& query)
{
    if (!bus.isValid())
    {
        return false;
    }

    const QString             actual_query{QStringLiteral("Can") + query};
    const QDBusReply<QString> reply{bus.call(QDBus::Block, actual_query)};
    if (!reply.isValid())
    {
        qCWarning(lc::os).nospace() << "got invalid reply for " << log_entry << " (" << actual_query
                                    << "): " << reply.error();
        return false;
    }

    if (reply.value() != QStringLiteral("yes"))
    {
        qCWarning(lc::os).nospace() << "got unexpected reply for " << log_entry << " (" << actual_query
                                    << "): " << reply.value();
        return false;
    }

    return true;
}

bool doQuery(QDBusInterface& bus, const QString& log_entry, const QString& query)
{
    if (!canDoQuery(bus, log_entry, query))
    {
        return false;
    }

    constexpr bool polkit_interactive{true};
    if (const QDBusReply<void> reply{bus.call(QDBus::Block, query, polkit_interactive)}; !reply.isValid())
    {
        qCWarning(lc::os).nospace() << "got invalid reply for " << log_entry << " (" << query << "): " << reply.error();
        return false;
    }

    return true;
}
}  // namespace

namespace os
{
namespace internal
{
Login1Manager::Login1Manager()
    : QDBusInterface{"org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager",
                     QDBusConnection::systemBus()}
{
}
}  // namespace internal

NativePcStateHandler::NativePcStateHandler()
{
    if (!m_login1_bus.isValid())
    {
        qCWarning(lc::os) << "logind bus is invalid!";
    }

    connect(&m_login1_bus, &internal::Login1Manager::PrepareForSleep, this,
            [this](const bool going_to_sleep)
            {
                if (!going_to_sleep)
                {
                    emit signalWokeUp();
                }
            });
}

bool NativePcStateHandler::canShutdownPC()
{
    return canDoQuery(m_login1_bus, "shutdown", "PowerOff");
}

bool NativePcStateHandler::canRestartPC()
{
    return canDoQuery(m_login1_bus, "restart", "Reboot");
}

bool NativePcStateHandler::canSuspendPC()
{
    return canDoQuery(m_login1_bus, "suspend", "Suspend");
}

bool NativePcStateHandler::canHibernatePC()
{
    return canDoQuery(m_login1_bus, "hibernate", "Hibernate");
}

bool NativePcStateHandler::shutdownPC()
{
    return doQuery(m_login1_bus, "shutdown", "PowerOff");
}

bool NativePcStateHandler::restartPC()
{
    return doQuery(m_login1_bus, "restart", "Reboot");
}

bool NativePcStateHandler::suspendPC()
{
    return doQuery(m_login1_bus, "suspend", "Suspend");
}

bool NativePcStateHandler::hibernatePC()
{
    return doQuery(m_login1_bus, "hibernate", "Hibernate");
}
}  // namespace os
