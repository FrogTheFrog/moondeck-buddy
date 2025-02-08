#pragma once

// local includes
#include "os/steam/steamcontentlogtracker.h"
#include "os/steam/steamprocesstracker.h"

// forward declarations
namespace utils
{
class AppSettings;
}  // namespace utils
namespace os
{
class SteamWebHelperLogTracker;
class SteamContentLogTracker;
}  // namespace os

namespace os
{
class SteamHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SteamHandler)

public:
    explicit SteamHandler(const utils::AppSettings&            app_settings,
                          std::unique_ptr<SteamProcessTracker> steam_process_tracker);
    ~SteamHandler() override;

    bool isSteamReady() const;
    bool close();

    bool launchApp(uint app_id);

signals:
    void signalSteamClosed();

private slots:
    void slotSteamProcessStateChanged();

private:
    struct LogTrackers
    {
        std::unique_ptr<SteamWebHelperLogTracker> m_web_helper;
        std::unique_ptr<SteamContentLogTracker>   m_content_log;
    };

    const utils::AppSettings&            m_app_settings;
    std::unique_ptr<SteamProcessTracker> m_steam_process_tracker;
    LogTrackers                          m_log_trackers;
};
}  // namespace os
