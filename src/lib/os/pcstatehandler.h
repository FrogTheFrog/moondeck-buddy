#pragma once

// system/Qt includes
#include <QObject>
#include <QTimer>

// local includes
#include "nativepcstatehandlerinterface.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class PcStateHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(PcStateHandler)

public:
    explicit PcStateHandler(std::unique_ptr<NativePcStateHandlerInterface> native_handler);
    ~PcStateHandler() override = default;

    shared::PcState getState() const;

    bool shutdownPC(uint grace_period_in_sec);
    bool restartPC(uint grace_period_in_sec);
    bool suspendPC(uint grace_period_in_sec);

public slots:
    void slotResetState();

private:
    using NativeMethod = bool (NativePcStateHandlerInterface::*)();
    bool doChangeState(uint grace_period_in_sec, const QString& cant_do_entry, const QString& failed_to_do_entry,
                       NativeMethod can_do_method, NativeMethod do_method, shared::PcState new_state);

    QTimer                                         m_state_change_back_timer;
    shared::PcState                                m_state{shared::PcState::Normal};
    std::unique_ptr<NativePcStateHandlerInterface> m_native_handler;
};
}  // namespace os