#include "steam/steamgameprocesslogtracker.h"
#include <QCoreApplication>
#include <QTemporaryDir>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition)
    {
        qFatal("%s", message);
    }
}

class TestTracker : public steam::SteamGameProcessLogTracker
{
public:
    using SteamGameProcessLogTracker::onLogChanged;
    using SteamGameProcessLogTracker::SteamGameProcessLogTracker;
};
}  // namespace

int main(int argc, char** argv)
{
    QCoreApplication app{argc, argv};
    QTemporaryDir    directory;
    require(directory.isValid(), "Temporary log directory should be available");
    TestTracker        tracker{directory.path().toStdString(), {}};
    const steam::AppId native{728880};
    const steam::AppId shortcut{11586750990639955968ULL};
    const auto         first{QDateTime::fromString("2026-09-11 10:00:00", "yyyy-MM-dd HH:mm:ss")};
    const auto         later{first.addSecs(60)};

    tracker.onLogChanged({"[2026-09-11 10:00:00] AppID 728880 adding PID 111"});
    require(tracker.getTrackedProcesses(native) == std::map<uint, QDateTime>{{111, first}},
            "Native game processes should retain their registration time");
    require(tracker.getTrackedProcesses(shortcut).empty(), "Unrelated apps must not inherit processes");

    tracker.onLogChanged({"[2026-09-11 10:01:00] AppID 11586750990639955968 adding PID 222"});
    require(tracker.getTrackedProcesses(shortcut) == std::map<uint, QDateTime>{{222, later}},
            "Full 64-bit non-Steam IDs should use the same process tracking path");

    tracker.onLogChanged({"[2026-09-11 10:01:00] AppID 11586750990639955968 adding PID 111"});
    require(tracker.getTrackedProcesses(native).at(111) == first,
            "A PID registered to another app must not refresh the old app's timestamp");
    require(tracker.getTrackedProcesses(shortcut).at(111) == later,
            "Process registration timestamps must belong to the app and PID pair");

    tracker.onLogChanged({"[2026-09-11 10:02:00] AppID 728880 no longer tracking PID 111"});
    require(tracker.getTrackedProcesses(native).empty(), "Removed processes must not remain stop targets");
    require(tracker.getTrackedProcesses(shortcut).size() == 1, "Other live processes must remain tracked");
    tracker.onLogChanged({"[2026-09-11 10:02:00] Game 11586750990639955968 going away, PID 222"});
    require(tracker.getTrackedProcesses(shortcut).empty(), "Game exit must clear its stop targets");

    tracker.onLogChanged({"[invalid timestamp ] AppID 728880 adding PID 333"});
    require(!tracker.getTrackedProcesses(native).at(333).isValid(),
            "Missing timestamps must remain invalid for stop target validation");
    qInfo("PASS: native/64-bit shortcut tracking, per-app timestamps, removal and invalid timestamps");
}
