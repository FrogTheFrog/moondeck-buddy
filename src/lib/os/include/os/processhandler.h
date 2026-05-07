#pragma once

// local includes
#include "common/enums.h"

// forward declarations
namespace os
{
class NativeProcessHandlerInterface;
}

namespace os
{
class ProcessHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ProcessHandler)

public:
    explicit ProcessHandler();
    ~ProcessHandler() override;

    std::vector<uint> getPids() const;
    QString           getExecPath(uint pid) const;
    QDateTime         getStartTime(uint pid) const;
    void              close(uint pid) const;
    void              terminate(uint pid) const;

private:
    std::unique_ptr<NativeProcessHandlerInterface> m_native_handler;
};
}  // namespace os
