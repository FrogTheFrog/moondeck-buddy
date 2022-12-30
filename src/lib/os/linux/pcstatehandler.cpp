// header file include
#include "pcstatehandler.h"

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
// NOLINTNEXTLINE(*-static)
shared::PcState PcStateHandler::getState() const
{
    qCCritical(lc::os) << Q_FUNC_INFO << ": not implemented!";
    return {};
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-static)
bool PcStateHandler::shutdownPC(uint grace_period_in_sec)
{
    Q_UNUSED(grace_period_in_sec)
    qCCritical(lc::os) << Q_FUNC_INFO << ": not implemented!";
    return false;
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-static)
bool PcStateHandler::restartPC(uint grace_period_in_sec)
{
    Q_UNUSED(grace_period_in_sec)
    qCCritical(lc::os) << Q_FUNC_INFO << ": not implemented!";
    return false;
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-static)
bool PcStateHandler::suspendPC(uint grace_period_in_sec)
{
    Q_UNUSED(grace_period_in_sec)
    qCCritical(lc::os) << Q_FUNC_INFO << ": not implemented!";
    return false;
}
}  // namespace os
