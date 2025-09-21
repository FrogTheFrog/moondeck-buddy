// header file include
#include "os/systemtray.h"

#if defined(Q_OS_LINUX)
// system/Qt includes
    #include <QtDBus/QDBusInterface>
#endif

// local includes
#include "os/pccontrol.h"
#include "shared/loggingcategories.h"

namespace os
{
SystemTray::SystemTray(const QIcon& icon, QString app_name, PcControl& pc_control)
    : m_autostart_action{"Start on system startup"}
    , m_quit_action{"Exit"}
    , m_icon{icon}
    , m_app_name{std::move(app_name)}
    , m_pc_control{pc_control}
{
    connect(&m_autostart_action, &QAction::triggered, this,
            [this]()
            {
                m_pc_control.setAutoStart(m_autostart_action.isChecked());
                if (m_autostart_action.isChecked() != m_pc_control.isAutoStartEnabled())
                {
                    qCWarning(lc::utils) << "failed to enable/disable autostart!";
                    QTimer::singleShot(0, this,
                                       [this]() { m_autostart_action.setChecked(m_pc_control.isAutoStartEnabled()); });
                }
                else if (m_pc_control.isServiceSupported() && m_autostart_action.isChecked())
                {
                    QTimer::singleShot(0, this, [this]() { emit signalRestartIntoService(); });
                }
            });
    connect(&m_menu, &QMenu::aboutToShow, this,
            [this]()
            {
                // Sync the state in case it was set via cmd
                m_autostart_action.setChecked(m_pc_control.isAutoStartEnabled());
            });
    connect(&m_quit_action, &QAction::triggered, this, &SystemTray::signalQuitApp);
    connect(&m_tray_attach_retry_timer, &QTimer::timeout, this, &SystemTray::slotTryAttach);

    m_autostart_action.setCheckable(true);

    m_menu.addAction(&m_autostart_action);
    m_menu.addAction(&m_quit_action);

    const auto retry_interval{5000};
    m_tray_attach_retry_timer.setInterval(retry_interval);
    m_tray_attach_retry_timer.setSingleShot(true);
    slotTryAttach();
}

void SystemTray::slotShowTrayMessage(const QString& title, const QString& message, QSystemTrayIcon::MessageIcon icon,
                                     int millisecondsTimeoutHint)
{
    if (!m_tray_icon || millisecondsTimeoutHint <= 0)
    {
        return;
    }

    m_tray_icon->showMessage(title, message, icon, millisecondsTimeoutHint);
}

void SystemTray::slotTryAttach()
{
    Q_ASSERT(m_tray_icon == nullptr);

#if defined(Q_OS_LINUX)
    // workaround for https://bugreports.qt.io/browse/QTBUG-94871
    const QDBusInterface systrayHost(QLatin1String("org.kde.StatusNotifierWatcher"),
                                     QLatin1String("/StatusNotifierWatcher"),
                                     QLatin1String("org.kde.StatusNotifierWatcher"));
    if (!systrayHost.isValid() || !systrayHost.property("IsStatusNotifierHostRegistered").toBool())
    {
        constexpr int max_retries{25};
        if (m_retry_counter++ < max_retries)
        {
            m_tray_attach_retry_timer.start();
            return;
        }
    }
#endif

    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        qCWarning(lc::utils) << "failed to initialize system tray...";
        return;
    }

    m_tray_icon = std::make_unique<QSystemTrayIcon>();
    m_tray_icon->setIcon(m_icon);
    m_tray_icon->setContextMenu(&m_menu);
    m_tray_icon->setVisible(true);
    m_tray_icon->setToolTip(m_app_name);
}
}  // namespace os
