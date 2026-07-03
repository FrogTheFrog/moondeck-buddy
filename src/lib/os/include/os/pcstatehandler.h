#pragma once

// system/Qt includes
#include <QTimer>
#include <QtWidgets/QSystemTrayIcon>

// local includes
#include "common/enums.h"

// forward declarations
namespace os
{
class NativePcStateHandlerInterface;
}

namespace os
{
class PcStateHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(PcStateHandler)

public:
    explicit PcStateHandler();
    ~PcStateHandler() override;

    enums::PcState getState() const;

    bool shutdownPC(uint grace_period_in_sec);
    bool restartPC(uint grace_period_in_sec);
    bool suspendPC(uint grace_period_in_sec);
    bool hibernatePC(uint grace_period_in_sec);
    bool abortPcStateChange();

signals:
    void signalTransientPcState();
    void signalShowTrayMessage(const QString& title, const QString& message, QSystemTrayIcon::MessageIcon icon,
                               int milliseconds_timeout_hint);

private:
    using NativeMethod = bool (NativePcStateHandlerInterface::*)();
    bool doChangeState(uint grace_period_in_sec, const QString& cant_do_entry, const QString& failed_to_do_entry,
                       NativeMethod can_do_method, NativeMethod do_method, enums::PcState new_state);

    enums::PcState                                 m_state{enums::PcState::Normal};
    QTimer                                         m_grace_timer;
    std::function<void()>                          m_pending_change;
    std::unique_ptr<NativePcStateHandlerInterface> m_native_handler;
};
}  // namespace os
