// header file include
#include "os/linux/nativeautostarthandler.h"

// system/Qt includes
#include <QDir>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

// local includes
#include "shared/appmetadata.h"
#include "shared/loggingcategories.h"

namespace
{
// QString getAutoStartContents(const shared::AppMetadata& app_meta)
// {
//     QString     contents;
//     QTextStream stream(&contents);
//
//     stream << "[Desktop Entry]" << Qt::endl;
//     stream << "Type=Application" << Qt::endl;
//     stream << "Name=" << app_meta.getAppName() << Qt::endl;
//     stream << "Exec=" << app_meta.getAutoStartExec() << Qt::endl;
//     stream << "Icon=" << app_meta.getAppName() << Qt::endl;
//
//     return contents;
// }
}  // namespace

namespace os
{
NativeAutoStartHandler::NativeAutoStartHandler(const shared::AppMetadata& app_meta)
    : m_app_meta{app_meta}
{
}

void NativeAutoStartHandler::setAutoStart(const bool enable)
{
    Q_UNUSED(enable);
    // const auto dir{m_app_meta.getAutoStartDir(shared::AppMetadata::AutoStartDelegation::V1)};
    // QFile      file{m_app_meta.getAutoStartPath(shared::AppMetadata::AutoStartDelegation::V1)};
    //
    // if (file.exists() && !file.remove())
    // {
    //     qFatal("Failed to remove %s",
    //            qUtf8Printable(m_app_meta.getAutoStartPath(shared::AppMetadata::AutoStartDelegation::V1)));
    //     return;
    // }
    //
    // if (enable)
    // {
    //     const QDir autostart_dir;
    //     if (!autostart_dir.mkpath(dir))
    //     {
    //         qFatal("Failed at mkpath %s", qUtf8Printable(dir));
    //         return;
    //     }
    //
    //     if (!file.open(QIODevice::WriteOnly))
    //     {
    //         qFatal("Failed to open %s",
    //                qUtf8Printable(m_app_meta.getAutoStartPath(shared::AppMetadata::AutoStartDelegation::V1)));
    //         return;
    //     }
    //
    //     file.write(getAutoStartContents(m_app_meta).toUtf8());
    // }
}

bool NativeAutoStartHandler::isAutoStartEnabled() const
{
    QDBusInterface manager_bus("org.freedesktop.systemd1",          // service name
                               "/org/freedesktop/systemd1",         // object path
                               "org.freedesktop.systemd1.Manager",  // interface name
                               QDBusConnection::systemBus());
    if (!manager_bus.isValid())
    {
        qFatal("Could not create QDBusInterface instance for managing services!");
        return false;
    }

    const auto                unit_name{m_app_meta.getAutoStartName(shared::AppMetadata::AutoStartDelegation::V2)};
    const QDBusReply<QString> reply{manager_bus.call("GetUnitFileState", unit_name)};
    if (!reply.isValid())
    {
        qCInfo(lc::os()) << "Service unit" << unit_name << "does not exist.";
        return false;
    }

    qCInfo(lc::os()) << "Service unit" << unit_name << "has the following state:" << reply.value();
    return false;
}
}  // namespace os
