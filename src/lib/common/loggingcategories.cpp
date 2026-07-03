// header file include
#include "common/loggingcategories.h"

namespace lc
{
Q_LOGGING_CATEGORY(buddyMain, "buddy.main", QtInfoMsg);
Q_LOGGING_CATEGORY(streamMain, "buddy.stream", QtInfoMsg);
Q_LOGGING_CATEGORY(common, "buddy.common", QtInfoMsg);
Q_LOGGING_CATEGORY(server, "buddy.server", QtInfoMsg);
Q_LOGGING_CATEGORY(steam, "buddy.steam", QtInfoMsg);
Q_LOGGING_CATEGORY(steamVerbose, "buddy.verbose.steam", QtInfoMsg);
Q_LOGGING_CATEGORY(utils, "buddy.utils", QtInfoMsg);
Q_LOGGING_CATEGORY(os, "buddy.os", QtInfoMsg);
Q_LOGGING_CATEGORY(osVerbose, "buddy.verbose.os", QtInfoMsg);
}  // namespace lc