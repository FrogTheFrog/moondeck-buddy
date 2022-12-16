// system/Qt includes
#include <QApplication>

// local includes
#include "shared/constants.h"
#include "utils/singleinstanceguard.h"

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-avoid-c-arrays)
int main(int argc, char* argv[])
{
    utils::SingleInstanceGuard guard{shared::APP_NAME_STREAM};
    if (!guard.tryToRun())
    {
        return EXIT_SUCCESS;
    }

    const QApplication app{argc, argv};
    return QCoreApplication::exec();
}
