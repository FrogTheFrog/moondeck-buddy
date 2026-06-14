// header file include
#include "steam/steamprocesstracker.h"

// system/Qt includes
#include <QDir>
#include <QRegularExpression>
#include <algorithm>

// local includes
#include "common/loggingcategories.h"

namespace
{
std::filesystem::path getSteamDir(const QString& exec_path)
{
    QDir dir{exec_path};
    for (int i = 0; i < 3; ++i)  // Go up only 3 levels
    {
        static const QStringList required_dirs{"logs", "userdata", "steamui"};
        if (std::ranges::all_of(required_dirs, [&dir](const auto& path) { return dir.exists(path); }))
        {
            return dir.filesystemCanonicalPath();
        }

        if (!dir.cdUp())
        {
            break;
        }
    }

    return {};
}
}  // namespace

namespace steam
{
SteamProcessTracker::SteamProcessTracker()

{
    connect(&m_check_timer, &QTimer::timeout, this, &SteamProcessTracker::slotCheckState);

    m_check_timer.setInterval(1000);
    m_check_timer.setSingleShot(true);

    QTimer::singleShot(0, this, &SteamProcessTracker::slotCheckState);
}

// For forward declarations
SteamProcessTracker::~SteamProcessTracker() = default;

void SteamProcessTracker::close()
{
    slotCheckState();
    if (isRunning())
    {
#if defined(Q_OS_WIN)
        m_process_handler.terminate(m_data.m_pid);
#elif defined(Q_OS_LINUX)
        m_process_handler.close(m_data.m_pid);
#else
    #error OS is not supported!
#endif
    }
}

bool SteamProcessTracker::isRunning() const
{
    return m_data.m_pid != 0;
}

uint SteamProcessTracker::getPid() const
{
    return m_data.m_pid;
}

QDateTime SteamProcessTracker::getStartTime() const
{
    return m_data.m_start_time;
}

const SteamLogTrackers* SteamProcessTracker::getSteamLogTrackers() const
{
    return m_data.m_log_trackers.get();
}

std::filesystem::path SteamProcessTracker::getSteamDir() const
{
    return m_data.m_steam_dir;
}

void SteamProcessTracker::slotCheckState()
{
    m_check_timer.stop();
    const auto auto_start_timer{qScopeGuard([this]() { m_check_timer.start(); })};

    if (isRunning())
    {
        const auto start_time = m_process_handler.getStartTime(m_data.m_pid);
        if (m_data.m_start_time == start_time)
        {
            return;
        }

        m_data = {};
        emit signalProcessStateChanged();
    }

    const auto pids{m_process_handler.getPids()};
    for (const auto pid : pids)
    {
        const QString exec_path{m_process_handler.getExecPath(pid)};
        if (exec_path.isEmpty())
        {
            continue;
        }

        // clang-format off
        static const QRegularExpression exec_regex{
            R"((?:.+?steam\.exe$))"        // Windows
            R"(|)"                         // OR
            R"((?:.*?steam.+?steam$))",    // Linux
            QRegularExpression::CaseInsensitiveOption};
        // clang-format on

        if (!exec_path.contains(exec_regex))
        {
            continue;
        }
        qCInfo(lc::steam) << "Found a matching Steam process. PATH:" << exec_path << "| PID:" << pid;

        auto cleanup{qScopeGuard([this]() { m_data = {}; })};

        m_data.m_steam_dir = ::getSteamDir(exec_path);
        if (m_data.m_steam_dir.empty())
        {
            qCInfo(lc::steam) << "Could not resolve steam directory for running Steam process, PID:" << pid;
            continue;
        }

        const auto steam_log_dir{m_data.m_steam_dir / "logs"};
        if (!std::filesystem::exists(steam_log_dir))
        {
            qCInfo(lc::steam) << "Could not resolve steam logs directory for running Steam process, PID:" << pid;
            continue;
        }

        m_data.m_start_time = m_process_handler.getStartTime(pid);
        if (!m_data.m_start_time.isValid())
        {
            qCWarning(lc::steam) << "Could not resolve start time for running Steam process! PID:" << pid;
            break;
        }

        cleanup.dismiss();

        m_data.m_pid          = pid;
        m_data.m_log_trackers = std::make_unique<SteamLogTrackers>(steam_log_dir, m_data.m_start_time);

        emit signalProcessStateChanged();
        break;
    }
}
}  // namespace steam
