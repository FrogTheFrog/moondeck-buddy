#pragma once

// system/Qt includes
#include <QTimer>
#include <QtWidgets/QMenu>
#include <QtWidgets/QSystemTrayIcon>
#include <memory>

// forward declarations
namespace os
{
class PcControl;
}

namespace os
{
class SystemTray : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SystemTray)

public:
    explicit SystemTray(const QIcon& icon, QString app_name, PcControl& pc_control);
    ~SystemTray() override = default;

signals:
    void signalQuitApp();

public slots:
    void slotShowTrayMessage(const QString& title, const QString& message, QSystemTrayIcon::MessageIcon icon,
                             int millisecondsTimeoutHint);

private slots:
    void slotTryAttach();

private:
    // Note: ctor/dtor order is important!
    QAction                          m_autostart_action;
    QAction                          m_quit_action;
    QMenu                            m_menu;
    std::unique_ptr<QSystemTrayIcon> m_tray_icon;

    QTimer m_tray_attach_retry_timer;
    uint   m_retry_counter{0};

    const QIcon& m_icon;
    QString      m_app_name;
    PcControl&   m_pc_control;
};
}  // namespace os
