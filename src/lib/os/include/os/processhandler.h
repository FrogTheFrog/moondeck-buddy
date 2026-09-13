#pragma once

// system/Qt includes
#include <QDateTime>

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

    std::vector<uint>        getPids() const;
    std::optional<QString>   getExecPath(uint pid) const;
    std::optional<QDateTime> getStartTime(uint pid) const;
    std::optional<bool>      close(uint pid) const;
    std::optional<bool>      terminate(uint pid) const;

private:
    std::unique_ptr<NativeProcessHandlerInterface> m_native_handler;
};
}  // namespace os
