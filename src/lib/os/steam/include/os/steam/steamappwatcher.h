#pragma once

// system/Qt includes
#include <QTimer>

// local includes
#include "shared/enums.h"

// forward declarations
namespace os
{
class SteamProcessTracker;
}  // namespace os

namespace os
{
class SteamAppWatcher : public QObject
{
    Q_OBJECT

public:
    explicit SteamAppWatcher(const SteamProcessTracker& process_tracker, std::uint64_t app_id);
    ~SteamAppWatcher() override;

    static enums::AppState getAppState(const SteamProcessTracker& process_tracker, std::uint64_t app_id,
                                       enums::AppState prev_state = enums::AppState::Stopped);

    enums::AppState getAppState() const;
    std::uint64_t   getAppId() const;

private slots:
    void slotCheckState();

private:
    const SteamProcessTracker& m_process_tracker;
    std::uint64_t              m_app_id;

    enums::AppState m_current_state{enums::AppState::Stopped};
    QTimer          m_check_timer;
    uint            m_delay_counter{0};
};
}  // namespace os
