// header file include
#include "os/linux/nativepcstatehandler.h"

// system/Qt includes
#include <QtDBus/QDBusReply>

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------------------------------------------

bool doQuery(QDBusInterface& bus, const QString& log_entry, const QString& query)
{
    if (!canDoQuery(bus, log_entry, query))
    {
        return false;
    }

    const bool             polkit_interactive{true};
    const QDBusReply<void> reply{bus.call(QDBus::Block, query, polkit_interactive)};
    if (!reply.isValid())
    {
        qCWarning(lc::os).nospace() << "got invalid reply for " << log_entry << " (" << query << "): " << reply.error();
        return false;
    }

    return true;
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
NativePcStateHandler::NativePcStateHandler()
    : m_logind_bus{"org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager",
                   QDBusConnection::systemBus()}
{
    if (!m_logind_bus.isValid())
    {
        qCWarning(lc::os) << "logind bus is invalid!";
    }
}

//---------------------------------------------------------------------------------------------------------------------

bool NativePcStateHandler::canShutdownPC()
{
    return canDoQuery(m_logind_bus, "shutdown", "PowerOff");
}

//---------------------------------------------------------------------------------------------------------------------

bool NativePcStateHandler::canRestartPC()
{
    return canDoQuery(m_logind_bus, "restart", "Reboot");
}

//---------------------------------------------------------------------------------------------------------------------

bool NativePcStateHandler::canSuspendPC()
{
    return canDoQuery(m_logind_bus, "suspend", "Suspend");
}

//---------------------------------------------------------------------------------------------------------------------

bool NativePcStateHandler::canHibernatePC()
{
    return canDoQuery(m_logind_bus, "hibernate", "Hibernate");
}

//---------------------------------------------------------------------------------------------------------------------

bool NativePcStateHandler::shutdownPC()
{
    return doQuery(m_logind_bus, "shutdown", "PowerOff");
}

//---------------------------------------------------------------------------------------------------------------------

bool NativePcStateHandler::restartPC()
{
    return doQuery(m_logind_bus, "restart", "Reboot");
}

//---------------------------------------------------------------------------------------------------------------------

bool NativePcStateHandler::suspendPC()
{
    return doQuery(m_logind_bus, "suspend", "Suspend");
}

//---------------------------------------------------------------------------------------------------------------------

bool NativePcStateHandler::hibernatePC()
{
    return doQuery(m_logind_bus, "hibernate", "Hibernate");
}
}  // namespace os
