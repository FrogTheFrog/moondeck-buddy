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
}

//---------------------------------------------------------------------------------------------------------------------

void SteamProcessListObserver::observePid(uint pid)
{
    stopObserving();

    if (pid != 0)
    {
        m_steam_pid = pid;

        slotCheckProcessList();
        m_check_timer.start();
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
    NativeProcessHandler proc_handler;
    std::set<uint>       running_apps;

    const auto pids{proc_handler.getChildrenPids(m_steam_pid)};
    for (auto pid : pids)
    {
        static const QRegularExpression app_id_matcher{R"(AppId=([0-9]+))", QRegularExpression::CaseInsensitiveOption};
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

    if (running_apps != m_app_ids)
    {
        std::swap(running_apps, m_app_ids);
        emit signalListChanged();
    }
}
}  // namespace os
