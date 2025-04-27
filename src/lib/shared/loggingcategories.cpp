// header file include
#include "shared/loggingcategories.h"

namespace lc
{
Q_LOGGING_CATEGORY(buddyMain, "buddy.main", QtInfoMsg);
Q_LOGGING_CATEGORY(streamMain, "buddy.stream", QtInfoMsg);
Q_LOGGING_CATEGORY(server, "buddy.server", QtInfoMsg);
Q_LOGGING_CATEGORY(shared, "buddy.shared", QtInfoMsg);
Q_LOGGING_CATEGORY(utils, "buddy.utils", QtInfoMsg);
Q_LOGGING_CATEGORY(os, "buddy.os", QtInfoMsg);
Q_LOGGING_CATEGORY(osVerbose, "buddy.os.verbose", QtInfoMsg);
}  // namespace lc