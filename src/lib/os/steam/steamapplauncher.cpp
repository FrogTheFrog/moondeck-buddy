// header file include
#include "os/steam/steamapplauncher.h"

// system/Qt includes

// local includes

namespace os
{
SteamAppLauncher::SteamAppLauncher(const SteamProcessTracker& process_tracker, const uint app_id,
                                   const bool force_big_picture)
    : m_process_tracker{process_tracker}
    , m_app_id{app_id}
    , m_force_big_picture{force_big_picture}
{
}

void SteamAppLauncher::slotExecuteLaunch()
{
}
}  // namespace os
