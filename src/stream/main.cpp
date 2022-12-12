// system/Qt includes
#include <QApplication>

// local includes
#include "utils/singleinstanceguard.h"

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-avoid-c-arrays)
int main(int argc, char* argv[])
{
    utils::SingleInstanceGuard guard{"STREAM"};
    if (!guard.tryToRun())
    {
        return EXIT_SUCCESS;
    }

    QApplication app{argc, argv};

    return QCoreApplication::exec();
}
