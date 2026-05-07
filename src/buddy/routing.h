#pragma once

// local includes
#include "pccontrol.h"
#include "server/httpserver.h"
#include "server/pairingmanager.h"
#include "sunshineapps.h"

void setupRoutes(server::HttpServer& server, server::PairingManager& pairing_manager, PcControl& pc_control,
                 SunshineApps& sunshine_apps, const QString& mac_address_override);
