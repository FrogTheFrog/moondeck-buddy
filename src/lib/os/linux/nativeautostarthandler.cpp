// header file include
#include "os/linux/nativeautostarthandler.h"

// system/Qt includes
#include <QDir>
#include <QPair>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

// local includes
#include "shared/appmetadata.h"
#include "shared/loggingcategories.h"

namespace
{
// [Unit]
// Description=MoonDeck companion
// After=sunshine.service
// PartOf=graphical-session.target
// Requisite=graphical-session.target
//
// [Service]
// ExecStart=%h/Downloads/MoonDeckBuddy.AppImage
// Restart=on-failure
// RestartSec=10
//
// [Install]
// WantedBy=graphical-session.target
//
QString getAutoStartContents(const shared::AppMetadata& app_meta)
{
    QString     contents;
    QTextStream stream(&contents);

    stream << "[Unit]" << Qt::endl;
    stream << "Description=MoonDeck host companion" << Qt::endl;
    stream << Qt::endl;
    stream << "[Service]" << Qt::endl;
    stream << "ExecStart=" << app_meta.getAutoStartExec() << Qt::endl;
    stream << "Restart=on-failure" << Qt::endl;
    stream << "RestartSec=10" << Qt::endl;
    stream << Qt::endl;
    stream << "[Install]" << Qt::endl;
    stream << "WantedBy=graphical-session.target" << Qt::endl;

    return contents;
}
}  // namespace

// struct MyStructure
// {
//     QString m_unit_path;
//     QString name;
//
//     // ...
// };
// Q_DECLARE_METATYPE(MyStructure)
//
// // Marshall the MyStructure data into a D-Bus argument
// QDBusArgument &operator<<(QDBusArgument &argument, const MyStructure &myStruct)
// {
//     argument.beginStructure();
//     argument << myStruct.count << myStruct.name;
//     argument.endStructure();
//     return argument;
// }
//
// // Retrieve the MyStructure data from the D-Bus argument
// const QDBusArgument &operator>>(const QDBusArgument &argument, MyStructure &myStruct)
// {
//     argument.beginStructure();
//     argument >> myStruct.count >> myStruct.name;
//     argument.endStructure();
//     return argument;
// }

namespace os
{
NativeAutoStartHandler::NativeAutoStartHandler(const shared::AppMetadata& app_meta)
    : m_app_meta{app_meta}
{
}

void NativeAutoStartHandler::setAutoStart(const bool enable)
{
    QDBusInterface manager_bus("org.freedesktop.systemd1", "/org/freedesktop/systemd1",
                               "org.freedesktop.systemd1.Manager", QDBusConnection::sessionBus());
    if (!manager_bus.isValid())
    {
        qFatal("Could not create QDBusInterface instance for managing services!");
    }

    if (enable)
    {
        {
            const auto dir{m_app_meta.getAutoStartDir(shared::AppMetadata::AutoStartDelegation::V2)};
            if (const QDir autostart_dir; !autostart_dir.mkpath(dir))
            {
                qFatal("Failed at mkpath %s", qUtf8Printable(dir));
                return;
            }

            QFile file{m_app_meta.getAutoStartPath(shared::AppMetadata::AutoStartDelegation::V2)};
            if (!file.open(QIODevice::WriteOnly))
            {
                qFatal("Failed to open %s", qUtf8Printable(file.fileName()));
                return;
            }

            file.write(getAutoStartContents(m_app_meta).toUtf8());
        }

        {
            const QString  method{"EnableUnitFiles"};
            const QVector  files{m_app_meta.getAutoStartPath(shared::AppMetadata::AutoStartDelegation::V2)};
            constexpr bool runtime{false};
            constexpr bool force{true};

            const QDBusReply<void> reply{manager_bus.call(QDBus::Block, method, files, runtime, force)};
            if (!reply.isValid())
            {
                qCWarning(lc::os()) << "EnableUnitFiles request failed -" << reply.error();
                return;
            }
        }

        // qCInfo(lc::os()) << "Service unit" << unit_name << "has the following state:" << reply.value();
    }
    else
    {
        {
            const QString  method{"DisableUnitFiles"};
            const QVector  files{m_app_meta.getAutoStartName(shared::AppMetadata::AutoStartDelegation::V2)};
            constexpr bool runtime{false};

            const QDBusReply<void> reply{manager_bus.call(QDBus::Block, method, files, runtime)};
            if (!reply.isValid())
            {
                qCWarning(lc::os()) << "DisableUnitFiles request failed -" << reply.error();
                return;
            }
        }
    }

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
                               QDBusConnection::sessionBus());
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

    qCInfo(lc::os()) << "Service unit has the following state:" << reply.value();
    return reply.value() == QStringLiteral("enabled");
}
}  // namespace os
