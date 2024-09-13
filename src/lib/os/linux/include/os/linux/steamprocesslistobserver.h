#pragma once

// system/Qt includes
#include <QTimer>
#include <set>

namespace os
{
class SteamProcessListObserver : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SteamProcessListObserver)

public:
    explicit SteamProcessListObserver();
    ~SteamProcessListObserver() override = default;

    uint findSteamProcess(uint previous_pid) const;
    void observePid(uint pid);
    void stopObserving();

    const std::set<uint>& getAppIds() const;

signals:
    void signalListChanged();

private slots:
    void slotCheckProcessList();

private:
    std::set<uint> m_app_ids;
    QTimer         m_check_timer;
    uint           m_steam_pid{0};
};
}  // namespace os
