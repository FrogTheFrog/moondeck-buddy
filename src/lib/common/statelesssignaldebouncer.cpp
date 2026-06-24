// header file include
#include "common/statelesssignaldebouncer.h"

// system/Qt includes
#include <QTimer>

namespace common
{
StatelessSignalDebouncer::StatelessSignalDebouncer(const int debounce_time_ms)
{
    connect(this, &StatelessSignalDebouncer::signalInput, this,
            [this, debounce_time_ms]()
            {
                if (!m_pending)
                {
                    m_pending = true;
                    QTimer::singleShot(debounce_time_ms, this,
                                       [this]()
                                       {
                                           m_pending = false;
                                           emit signalOutput();
                                       });
                }
            });
}
}  // namespace common