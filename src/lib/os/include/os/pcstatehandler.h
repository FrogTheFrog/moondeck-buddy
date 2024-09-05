#pragma once

// local includes
#include "shared/enums.h"

// forward declarations
namespace os
{
class NativePcStateHandlerInterface;
}

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class PcStateHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(PcStateHandler)

public:
    explicit PcStateHandler(std::unique_ptr<NativePcStateHandlerInterface> native_handler);
    ~PcStateHandler() override;

    enums::PcState getState() const;

    bool shutdownPC(uint grace_period_in_sec);
    bool restartPC(uint grace_period_in_sec);
    bool suspendPC(uint grace_period_in_sec);
    bool hibernatePC(uint grace_period_in_sec);

private:
    using NativeMethod = bool (NativePcStateHandlerInterface::*)();
    bool doChangeState(uint grace_period_in_sec, const QString& cant_do_entry, const QString& failed_to_do_entry,
                       NativeMethod can_do_method, NativeMethod do_method, enums::PcState new_state);

    enums::PcState                                 m_state{enums::PcState::Normal};
    std::unique_ptr<NativePcStateHandlerInterface> m_native_handler;
};
}  // namespace os
