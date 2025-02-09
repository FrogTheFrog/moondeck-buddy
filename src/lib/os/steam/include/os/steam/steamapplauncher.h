#pragma once

// system/Qt includes
#include <QObject>

// forward declarations
namespace os
{
class SteamProcessTracker;
class SteamWebHelperLogTracker;
}  // namespace os

namespace os
{
class SteamAppLauncher : public QObject
{
    Q_OBJECT

public:
    explicit SteamAppLauncher(const SteamProcessTracker& process_tracker, uint app_id, bool force_big_picture);
    ~SteamAppLauncher() override = default;

private slots:
    void slotExecuteLaunch();

private:
    const SteamProcessTracker& m_process_tracker;
    uint                       m_app_id;
    bool                       m_force_big_picture;
};
}  // namespace os
