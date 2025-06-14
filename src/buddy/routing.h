#pragma once

// local includes
#include "os/pccontrol.h"
#include "server/httpserver.h"
#include "server/pairingmanager.h"

void setupRoutes(server::HttpServer& server, server::PairingManager& pairing_manager, os::PcControl& pc_control,
                 const QString& mac_address_override);
