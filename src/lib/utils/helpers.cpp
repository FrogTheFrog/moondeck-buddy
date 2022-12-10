// header file include
#include "helpers.h"

//---------------------------------------------------------------------------------------------------------------------

namespace utils
{
const QString& getExecDir()
{
    static const QString dir{QCoreApplication::applicationDirPath() + "/"};
    return dir;
}
}  // namespace utils
