// header file include
#include "systemtray.h"

// local includes
#include "os/pccontrol.h"

//---------------------------------------------------------------------------------------------------------------------

namespace utils
{
SystemTray::SystemTray(const QIcon& icon, const QString& app_name, os::PcControl& pc_control)
    : m_autostart_action{"Start on system startup"}
    , m_quit_action{"Exit"}
    , m_pc_control{pc_control}
{
    QObject::connect(&m_autostart_action, &QAction::triggered, this,
                     [this]()
                     {
                         m_pc_control.setAutoStart(!m_pc_control.isAutoStartEnabled());
                         m_autostart_action.setChecked(m_pc_control.isAutoStartEnabled());
                     });
    QObject::connect(&m_quit_action, &QAction::triggered, this, &SystemTray::signalQuitApp);

    m_autostart_action.setCheckable(true);
    m_autostart_action.setChecked(m_pc_control.isAutoStartEnabled());

    m_menu.addAction(&m_autostart_action);
    m_menu.addAction(&m_quit_action);

    m_tray_icon.setIcon(icon);
    m_tray_icon.setContextMenu(&m_menu);
    m_tray_icon.setVisible(true);
    m_tray_icon.setToolTip(app_name);
}

//---------------------------------------------------------------------------------------------------------------------

void SystemTray::slotShowTrayMessage(const QString& title, const QString& message, QSystemTrayIcon::MessageIcon icon,
                                     int millisecondsTimeoutHint)
{
    m_tray_icon.showMessage(title, message, icon, millisecondsTimeoutHint);
}
}  // namespace utils
