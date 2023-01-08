// header file include
#include "steamhandler.h"

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
// NOLINTNEXTLINE(*-static)
bool SteamHandler::isRunning() const
{
    qCCritical(lc::os) << Q_FUNC_INFO << ": not implemented!";
    return false;
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-static)
bool SteamHandler::isRunningNow()
{
    qCCritical(lc::os) << Q_FUNC_INFO << ": not implemented!";
    return false;
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-static)
bool SteamHandler::close(std::optional<uint> grace_period_in_sec)
{
    Q_UNUSED(grace_period_in_sec)
    qCCritical(lc::os) << Q_FUNC_INFO << ": not implemented!";
    return false;
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-static)
bool SteamHandler::launchApp(uint app_id)
{
    Q_UNUSED(app_id)
    qCCritical(lc::os) << Q_FUNC_INFO << ": not implemented!";
    return false;
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-static)
uint SteamHandler::getRunningApp() const
{
    qCCritical(lc::os) << Q_FUNC_INFO << ": not implemented!";
    return 0;
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-static)
std::optional<uint> SteamHandler::getTrackedUpdatingApp() const
{
    qCCritical(lc::os) << Q_FUNC_INFO << ": not implemented!";
    return std::nullopt;
}
}  // namespace os
