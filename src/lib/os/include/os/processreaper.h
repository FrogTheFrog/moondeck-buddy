#pragma once

// system/Qt includes
#include <QDateTime>
#include <QTimer>
#include <map>
#include <set>

namespace os
{
class ProcessReaper : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ProcessReaper)

public:
    struct PidData
    {
        bool                   m_wait_to_end_only{false};
        QDateTime              m_timestamp;
        std::optional<QString> m_exec_path;
    };

    using PidsWithData = std::map<uint, PidData>;
    static PidsWithData preparePidData(const std::set<uint>& pids, bool with_exec_path = false);

    explicit ProcessReaper(const std::set<uint>& pids);
    explicit ProcessReaper(PidsWithData pids_with_timestamps);
    ~ProcessReaper() override = default;

    bool start();

signals:
    void signalFinishedReaping(const PidsWithData& failed_to_reap);

private slots:
    void slotPerformReaping();

private:
    bool         m_already_reaping{false};
    PidsWithData m_pids_with_data;
    QTimer       m_timer;
    int          m_repeat_counter{0};
};
}  // namespace os
