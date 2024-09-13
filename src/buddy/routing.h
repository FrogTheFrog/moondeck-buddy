#pragma once

// local includes
#include "os/pccontrol.h"
#include "os/sunshineapps.h"
#include "server/httpserver.h"
#include "server/pairingmanager.h"

void setupRoutes(server::HttpServer& server, server::PairingManager& pairing_manager, os::PcControl& pc_control,
                 os::SunshineApps& sunshine_apps, bool prefer_hibernation, bool force_big_picture,
                 bool close_steam_before_sleep, const QString& mac_address_override);
