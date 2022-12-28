// header file include
#include "helpers.h"

// system/Qt includes
#include <QCoreApplication>

//---------------------------------------------------------------------------------------------------------------------

namespace utils
{
const QString& getExecDir()
{
    static const QString dir{QCoreApplication::applicationDirPath() + "/"};
    return dir;
}
}  // namespace utils
