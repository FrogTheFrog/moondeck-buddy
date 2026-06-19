#pragma once

// local includes
#include "pccontrol.h"
#include "server/restserver.h"
#include "server/pairingmanager.h"
#include "sunshineapps.h"

void setupRoutes(server::RestServer& server, server::PairingManager& pairing_manager, PcControl& pc_control,
                 SunshineApps& sunshine_apps, const QString& mac_address_override);
