#pragma once

// local includes
#include "pccontrol.h"
#include "server/pairingmanager.h"
#include "server/restserver.h"

void setupRoutes(server::RestServer& server, server::PairingManager& pairing_manager, PcControl& pc_control,
                 const QString& mac_address_override);
