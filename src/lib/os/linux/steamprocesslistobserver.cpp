// header file include
#include "steamprocesslistobserver.h"

// system/Qt includes
#include <QRegularExpression>

// local includes
#include "nativeprocesshandler.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
SteamProcessListObserver::SteamProcessListObserver()
{
    connect(&m_check_timer, &QTimer::timeout, this, &SteamProcessListObserver::slotCheckProcessList);

    const int interval_ms{2000};
    m_check_timer.setInterval(interval_ms);
    m_check_timer.setSingleShot(true);
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-static)
uint SteamProcessListObserver::findSteamProcess() const
{
    NativeProcessHandler proc_handler;

    const auto pids{proc_handler.getPids()};
    for (const auto pid : pids)
    {
        const QString exec_path{proc_handler.getExecPath(pid)};
        if (exec_path.isEmpty())
        {
            continue;
        }

        static const QRegularExpression exec_regex{".*?Steam.+?steam", QRegularExpression::CaseInsensitiveOption};
        if (exec_path.contains(exec_regex))
        {
            return pid;
        }
    }

    return 0;
}

//---------------------------------------------------------------------------------------------------------------------

void SteamProcessListObserver::observePid(uint pid)
{
    stopObserving();

    if (pid != 0)
    {
        m_steam_pid = pid;
        slotCheckProcessList();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void SteamProcessListObserver::stopObserving()
{
    m_check_timer.stop();
    m_steam_pid = 0;
}

//---------------------------------------------------------------------------------------------------------------------

const std::set<uint>& SteamProcessListObserver::getAppIds() const
{
    return m_app_ids;
}

//---------------------------------------------------------------------------------------------------------------------

void SteamProcessListObserver::slotCheckProcessList()
{
    std::set<uint> running_apps;

    if (m_steam_pid != 0)
    {
        NativeProcessHandler proc_handler;

        const auto pids{proc_handler.getChildrenPids(m_steam_pid)};
        for (auto pid : pids)
        {
            static const QRegularExpression app_id_matcher{R"(AppId=([0-9]+))",
                                                           QRegularExpression::CaseInsensitiveOption};
            const auto                      match{app_id_matcher.match(proc_handler.getCmdline(pid))};
            if (!match.hasCaptured(1))
            {
                continue;
            }

            const QString captured{match.captured(1)};

            bool       success{false};
            const auto result{captured.toUInt(&success)};

            if (!success)
            {
                continue;
            }

            running_apps.insert(result);
        }
    }

    if (running_apps != m_app_ids)
    {
        std::swap(running_apps, m_app_ids);
        emit signalListChanged();
    }

    if (m_steam_pid != 0)
    {
        m_check_timer.start();
    }
}
}  // namespace os
