#pragma once

// system/Qt includes
#include <QObject>

// forward declarations
namespace os
{
class SteamProcessTracker;
}  // namespace os

namespace os
{
class SteamLauncher : public QObject
{
    Q_OBJECT

public:
    explicit SteamLauncher(const SteamProcessTracker& process_tracker, QString steam_exec, bool force_big_picture);
    ~SteamLauncher() override = default;

    static bool isSteamReady(const SteamProcessTracker& process_tracker, bool force_big_picture);
    void        setAppId(uint app_id);

signals:
    void signalFinished(const QString& steam_exec, uint app_id, bool success);

private slots:
    void slotExecuteLaunch();

private:
    enum class Stage
    {
        Initial,
        WaitingForSteam,
    };

    const SteamProcessTracker& m_process_tracker;
    QString                    m_steam_exec;
    bool                       m_force_big_picture;
    uint                       m_app_id{0};
    uint                       m_wait_counter{0};
    Stage                      m_stage{Stage::Initial};
};
}  // namespace os
