#pragma once

// system/Qt includes
#include <QtWidgets/QMenu>
#include <QtWidgets/QSystemTrayIcon>

// local includes
#include "os/pccontrol.h"

//---------------------------------------------------------------------------------------------------------------------

namespace utils
{
class SystemTray : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SystemTray)

public:
    explicit SystemTray(const QIcon& icon, const QString& app_name, os::PcControl& pc_control);
    ~SystemTray() override = default;

signals:
    void signalQuitApp();

public slots:
    void slotShowTrayMessage(const QString& title, const QString& message, QSystemTrayIcon::MessageIcon icon,
                             int millisecondsTimeoutHint);

private:
    // Note: ctor/dtor order is important!
    QAction         m_autostart_action;
    QAction         m_quit_action;
    QMenu           m_menu;
    QSystemTrayIcon m_tray_icon;
    os::PcControl&  m_pc_control;
};
}  // namespace utils
