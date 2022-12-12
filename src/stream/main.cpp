// system/Qt includes
#include <QApplication>

// local includes
#include "os/win/processtracker.h"
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

    bool buddy_running{false};
    bool streamer_running{false};

    os::ProcessTracker buddy_tracker(
        QRegularExpression{R"([\\/]MoonDeckBuddy\.exe$)", QRegularExpression::CaseInsensitiveOption});
    os::ProcessTracker streamer_tracker(
        QRegularExpression{R"([\\/]nvstreamer\.exe$)", QRegularExpression::CaseInsensitiveOption});

    const auto killer = [&]()
    {
        if (buddy_running && streamer_running)
        {
            qDebug() << "Killing in 7secs!";
            QTimer::singleShot(7000,
                               [&]()
                               {
                                   qDebug() << "Killing!";
                                   buddy_tracker.close();
                                   streamer_tracker.close();
                               });
        }
    };

    QObject::connect(&buddy_tracker, &os::ProcessTracker::signalProcessStateChanged,
                     [&](bool is_running)
                     {
                         buddy_running = is_running;
                         qDebug() << "BUDDY IS " << is_running;
                         killer();
                     });
    QObject::connect(&streamer_tracker, &os::ProcessTracker::signalProcessStateChanged,
                     [&](bool is_running)
                     {
                         streamer_running = is_running;
                         qDebug() << "STREAMER IS " << is_running;
                         killer();
                     });

    buddy_tracker.startObserving();
    streamer_tracker.startObserving();

    return QCoreApplication::exec();
}
