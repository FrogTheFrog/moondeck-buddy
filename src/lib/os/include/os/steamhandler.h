#pragma once

// local includes
#include "os/steam/steamprocesstracker.h"

// forward declarations
namespace utils
{
class AppSettings;
}  // namespace utils

namespace os
{
class SteamHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SteamHandler)

public:
    explicit SteamHandler(const utils::AppSettings&                      app_settings,
                          std::unique_ptr<NativeProcessHandlerInterface> process_handler_interface);
    ~SteamHandler() override;

    bool isSteamReady() const;
    bool close();

    bool launchApp(uint app_id);

signals:
    void signalSteamClosed();

private slots:
    void slotSteamProcessStateChanged();

private:
    const utils::AppSettings& m_app_settings;
    SteamProcessTracker       m_steam_process_tracker;
};
}  // namespace os
