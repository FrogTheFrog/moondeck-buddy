// header file include
#include "os/steam/steamprocesstracker.h"

// system/Qt includes
#include <QDir>
#include <QRegularExpression>

// local includes
#include "os/shared/nativeprocesshandlerinterface.h"
#include "shared/loggingcategories.h"

namespace
{
std::filesystem::path getLogsDir(const QString& exec_path)
{
    QDir dir{exec_path};
    for (int i = 0; i < 3; ++i)  // Go up only 3 levels
    {
        if (dir.exists("steamapps") && dir.exists("userdata"))
        {
            return QDir{dir.path() + "/logs"}.filesystemCanonicalPath();
        }

        if (!dir.cdUp())
        {
            break;
        }
    }

    return {};
}
}  // namespace

namespace os
{
SteamProcessTracker::SteamProcessTracker(std::unique_ptr<NativeProcessHandlerInterface> native_handler)
    : m_native_handler{std::move(native_handler)}
{
    Q_ASSERT(m_native_handler);

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
        m_native_handler->close(m_data.m_pid);
    }
}

void SteamProcessTracker::terminate()
{
    slotCheckState();
    if (isRunning())
    {
        m_native_handler->terminate(m_data.m_pid);
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

const SteamProcessTracker::LogTrackers* SteamProcessTracker::getLogTrackers() const
{
    return m_data.m_log_trackers.get();
}

void SteamProcessTracker::slotCheckState()
{
    m_check_timer.stop();
    const auto auto_start_timer{qScopeGuard([this]() { m_check_timer.start(); })};

    if (isRunning())
    {
        const auto start_time = m_native_handler->getStartTime(m_data.m_pid);
        if (m_data.m_start_time == start_time)
        {
            return;
        }

        m_data = {};
        emit signalProcessStateChanged();
    }

    const auto pids{m_native_handler->getPids()};
    for (const auto pid : pids)
    {
        const QString exec_path{m_native_handler->getExecPath(pid)};
        if (exec_path.isEmpty())
        {
            continue;
        }

        // clang-format off
        static const QRegularExpression exec_regex{
            R"((?:.+?Steam\\steam\.exe$))"  // Windows
            R"(|)"                         // OR
            R"((?:.*?Steam.+?steam$))",    // Linux
            QRegularExpression::CaseInsensitiveOption};
        // clang-format on

        if (!exec_path.contains(exec_regex))
        {
            continue;
        }

        auto cleanup{qScopeGuard([this]() { m_data = {}; })};

        m_data.m_log_dir = getLogsDir(exec_path);
        if (m_data.m_log_dir.empty())
        {
            qWarning(lc::os) << "Could not resolve log directory for running Steam process! PID:" << pid;
            break;
        }

        m_data.m_start_time = m_native_handler->getStartTime(pid);
        if (!m_data.m_start_time.isValid())
        {
            qWarning(lc::os) << "Could not resolve start time for running Steam process! PID:" << pid;
            break;
        }

        cleanup.dismiss();

        m_data.m_pid = pid;
        m_data.m_log_trackers.reset(new LogTrackers{SteamWebHelperLogTracker{m_data.m_log_dir, m_data.m_start_time},
                                                    SteamContentLogTracker{m_data.m_log_dir, m_data.m_start_time},
                                                    SteamGameProcessLogTracker{m_data.m_log_dir, m_data.m_start_time}});

        emit signalProcessStateChanged();
    }
}
}  // namespace os
