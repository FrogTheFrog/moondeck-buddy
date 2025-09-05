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
std::unique_ptr<QDBusInterface> getSystemdManager()
{
    auto bus_interface{std::make_unique<QDBusInterface>("org.freedesktop.systemd1", "/org/freedesktop/systemd1",
                                                        "org.freedesktop.systemd1.Manager",
                                                        QDBusConnection::sessionBus())};
    if (!bus_interface || !bus_interface->isValid())
    {
        qCWarning(lc::os()) << "Could not create QDBusInterface instance for managing services!";
    }

    return bus_interface;
}

QString getAutoStartContents(const shared::AppMetadata& app_meta, const shared::AppMetadata::AutoStartDelegation type)
{
    QString     contents;
    QTextStream stream(&contents);

    switch (type)
    {
        case shared::AppMetadata::AutoStartDelegation::Desktop:
            break;
        case shared::AppMetadata::AutoStartDelegation::ServiceMain:
        {
            stream << "[Unit]" << Qt::endl;
            stream << "Description=MoonDeck host companion" << Qt::endl;
            stream << "After=default.target" << Qt::endl;
            stream << Qt::endl;
            stream << "[Service]" << Qt::endl;
            stream << "Type=exec" << Qt::endl;
            stream << R"(ExecStart=/bin/sh -c 'NO_GUI="${MOONDECKBUDDY_NO_GUI:-true}" )" << app_meta.getAutoStartExec()
                   << "'" << Qt::endl;
            stream << "Restart=on-failure" << Qt::endl;
            stream << "RestartSec=10" << Qt::endl;
            stream << Qt::endl;
            stream << "[Install]" << Qt::endl;
            stream << "WantedBy=default.target" << Qt::endl;
            break;
        }
        case shared::AppMetadata::AutoStartDelegation::ServiceHelper:
        {
            stream << "[Unit]" << Qt::endl;
            stream << "Description=Restarts moondeckbuddy.service when leaving/entering DE environment" << Qt::endl;
            stream << "After=xdg-desktop-autostart.target" << Qt::endl;
            stream << "PartOf=xdg-desktop-autostart.target" << Qt::endl;
            stream << Qt::endl;
            stream << "[Service]" << Qt::endl;
            stream << "Type=exec" << Qt::endl;
            stream
                << R"(ExecStart=/bin/sh -c 'sleep infinity & PID=$!; trap "kill $PID" INT TERM; systemctl --user set-environment MOONDECKBUDDY_NO_GUI=false; systemctl --user try-restart )"
                << app_meta.getAutoStartName(shared::AppMetadata::AutoStartDelegation::ServiceMain)
                << R"(; wait; systemctl --user unset-environment MOONDECKBUDDY_NO_GUI; systemctl --user try-restart )"
                << app_meta.getAutoStartName(shared::AppMetadata::AutoStartDelegation::ServiceMain) << ";'" << Qt::endl;
            stream << "Restart=no" << Qt::endl;
            stream << Qt::endl;
            stream << "[Install]" << Qt::endl;
            stream << "WantedBy=xdg-desktop-autostart.target" << Qt::endl;
            break;
        }
    }

    return contents;
}

bool writeAutoStartContents(const shared::AppMetadata& app_meta, const shared::AppMetadata::AutoStartDelegation type)
{
    const auto dir{app_meta.getAutoStartDir(type)};
    if (const QDir autostart_dir; !autostart_dir.mkpath(dir))
    {
        qCWarning(lc::os()) << "Failed at mkpath for" << dir;
        return false;
    }

    QFile file{app_meta.getAutoStartPath(type)};
    if (!file.open(QIODevice::WriteOnly))
    {
        qCWarning(lc::os()) << "Failed to open" << file.fileName();
        return false;
    }

    const auto data{getAutoStartContents(app_meta, type).toUtf8()};
    if (file.write(data) != data.size())
    {
        qCWarning(lc::os()) << "Failed to write full contents to" << file.fileName();
        return false;
    }

    return true;
}

bool removeAutoStartContents(const shared::AppMetadata& app_meta, const shared::AppMetadata::AutoStartDelegation type)
{
    QFile file{app_meta.getAutoStartPath(type)};
    if (file.exists())
    {
        if (!file.remove())
        {
            qCWarning(lc::os()) << "Could not remove" << file.fileName();
            return false;
        }
    }

    return true;
}

bool hasSameAutoStartContents(const shared::AppMetadata& app_meta, const shared::AppMetadata::AutoStartDelegation type)
{
    QFile file{app_meta.getAutoStartPath(type)};
    if (file.exists())
    {
        if (file.open(QIODevice::ReadOnly))
        {
            return file.readAll() == getAutoStartContents(app_meta, type);
        }

        qCWarning(lc::os()) << "Could not open" << file.fileName() << "for reading - " << file.error();
    }

    return false;
}

bool isUnitFileEnabled(QDBusInterface& systemd_manager, const shared::AppMetadata& app_meta,
                       const shared::AppMetadata::AutoStartDelegation type)
{
    const QString method{"GetUnitFileState"};
    const auto    file{app_meta.getAutoStartName(type)};

    const QDBusReply<QString> reply{systemd_manager.call(QDBus::Block, method, file)};
    if (!reply.isValid())
    {
        qCDebug(lc::os()) << "Service unit" << file << "does not exist.";
        return false;
    }

    qCDebug(lc::os()) << "Service unit" << file << "has the following state:" << reply.value();
    return reply.value() == QStringLiteral("enabled");
}

QString getUnitFilePath(QDBusInterface& systemd_manager, const shared::AppMetadata& app_meta,
                        const shared::AppMetadata::AutoStartDelegation type)
{
    const QString object_path{
        [&systemd_manager, &app_meta, type]()
        {
            const QString method{"GetUnit"};
            const auto    name{app_meta.getAutoStartName(type)};

            const QDBusReply<QDBusObjectPath> reply{systemd_manager.call(QDBus::Block, method, name)};
            if (!reply.isValid())
            {
                qCWarning(lc::os()) << method << "request failed -" << reply.error();
                return QString{};
            }

            return reply.value().path();
        }()};
    if (object_path.isEmpty())
    {
        return {};
    }

    QDBusInterface unit_interface{"org.freedesktop.systemd1", object_path, "org.freedesktop.DBus.Properties",
                                  QDBusConnection::sessionBus()};
    if (!unit_interface.isValid())
    {
        qCWarning(lc::os()) << "Could not create QDBusInterface instance for unit" << object_path;
        return {};
    }

    const QString method{"Get"};
    const QString interface_name{"org.freedesktop.systemd1.Unit"};
    const QString property_name{"FragmentPath"};

    const QDBusReply<QDBusVariant> reply{unit_interface.call(QDBus::Block, method, interface_name, property_name)};
    if (!reply.isValid())
    {
        qCWarning(lc::os()) << method << "request failed -" << reply.error();
        return {};
    }

    return reply.value().variant().toString();
}

bool hasSameUnitFilePath(QDBusInterface& systemd_manager, const shared::AppMetadata& app_meta,
                         const shared::AppMetadata::AutoStartDelegation type)
{
    return getUnitFilePath(systemd_manager, app_meta, type) == app_meta.getAutoStartPath(type);
}

bool enableUnitFile(QDBusInterface& systemd_manager, const shared::AppMetadata& app_meta,
                    const shared::AppMetadata::AutoStartDelegation type)
{
    const QString  method{"EnableUnitFiles"};
    const QVector  files{app_meta.getAutoStartPath(type)};
    constexpr bool runtime{false};
    constexpr bool force{true};

    const QDBusReply<void> reply{systemd_manager.call(QDBus::Block, method, files, runtime, force)};
    if (!reply.isValid())
    {
        qCWarning(lc::os()) << method << "request failed -" << reply.error();
        return false;
    }

    return true;
}

bool disableUnitFile(QDBusInterface& systemd_manager, const shared::AppMetadata& app_meta,
                     const shared::AppMetadata::AutoStartDelegation type)
{
    const QString  method{"DisableUnitFiles"};
    const QVector  files{app_meta.getAutoStartName(type)};
    constexpr bool runtime{false};

    const QDBusReply<void> reply{systemd_manager.call(QDBus::Block, method, files, runtime)};
    if (!reply.isValid())
    {
        qCWarning(lc::os()) << method << "request failed -" << reply.error();
        return false;
    }

    return true;
}

void reloadDaemon(QDBusInterface& systemd_manager)
{
    const QString method{"Reload"};

    const QDBusReply<void> reply{systemd_manager.call(QDBus::Block, method)};
    if (!reply.isValid())
    {
        qCWarning(lc::os()) << method << "request failed -" << reply.error();
    }
}

}  // namespace

namespace os
{
NativeAutoStartHandler::NativeAutoStartHandler(const shared::AppMetadata& app_meta)
    : m_app_meta{app_meta}
{
}

void NativeAutoStartHandler::setAutoStart(const bool enable)
{
    using enum shared::AppMetadata::AutoStartDelegation;

    // Cleanup old startup file
    if (!removeAutoStartContents(m_app_meta, Desktop))
    {
        return;
    }

    const auto systemd_manager{getSystemdManager()};
    if (!systemd_manager)
    {
        return;
    }

    bool       reload_daemon{false};
    const auto reload_guard{qScopeGuard(
        [&reload_daemon, &systemd_manager]()
        {
            if (reload_daemon)
            {
                reloadDaemon(*systemd_manager);
            }
        })};

    // Any related units will do, we want to disable them anyway...
    for (const auto type : {ServiceMain, ServiceHelper})
    {
        if (isUnitFileEnabled(*systemd_manager, m_app_meta, type))
        {
            if (!disableUnitFile(*systemd_manager, m_app_meta, type))
            {
                return;
            }

            reload_daemon = true;
        }
    }

    for (const auto type : {ServiceMain, ServiceHelper})
    {
        if (enable)
        {
            if (!hasSameAutoStartContents(m_app_meta, type))
            {
                if (!writeAutoStartContents(m_app_meta, type))
                {
                    return;
                }
            }

            if (!enableUnitFile(*systemd_manager, m_app_meta, type))
            {
                return;
            }

            reload_daemon = true;
        }
        else
        {
            removeAutoStartContents(m_app_meta, type);
        }
    }
}

bool NativeAutoStartHandler::isAutoStartEnabled() const
{
    using enum shared::AppMetadata::AutoStartDelegation;

    const auto systemd_manager{getSystemdManager()};
    if (!systemd_manager)
    {
        return false;
    }

    static const std::vector types{ServiceMain, ServiceHelper};
    return std::ranges::all_of(types,
                               [&](const auto type)
                               {
                                   return isUnitFileEnabled(*systemd_manager, m_app_meta, type)
                                          && hasSameUnitFilePath(*systemd_manager, m_app_meta, type)
                                          && hasSameAutoStartContents(m_app_meta, type);
                               });
}
}  // namespace os
