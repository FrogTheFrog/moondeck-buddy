#pragma once

// system/Qt includes
#include <QObject>

// local includes
#include "processtracker.h"
#include "regkey.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class SteamHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SteamHandler)

public:
    explicit SteamHandler();
    ~SteamHandler() override = default;

    bool isRunning() const;
    bool isRunningNow();
    void close(std::optional<uint> grace_period_in_sec);

    void                launchApp(uint app_id, const QStringList& steam_args);
    uint                getRunningApp() const;
    std::optional<uint> isLastLaunchedAppUpdating() const;

signals:
    void signalSteamStarted();
    void signalSteamClosed();

private slots:
    void slotSteamProcessStateChanged();
    void slotHandleGlobalRegKeyChanges(const QMap<QString, QVariant>& changed_values);
    void slotHandleAppRegKeyChanges(const QMap<QString, QVariant>& changed_values, uint app_id);
    void slotForceCloseSteam();

private:
    struct TrackedAppData
    {
        uint m_app_id;
        bool m_is_running;
        bool m_is_updating;
    };

    ProcessTracker m_steam_process;
    RegKey         m_global_reg_key;
    QString        m_steam_exec_path;

    // Short-lived states
    std::unique_ptr<RegKey>       m_app_reg_key;
    std::optional<uint>           m_global_app_id;
    std::optional<TrackedAppData> m_tracked_app;
    QTimer                        m_steam_close_timer;
};
}  // namespace os
