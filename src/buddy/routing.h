#pragma once

// local includes
#include "os/pccontrolinterface.h"
#include "server/httpserver.h"
#include "server/pairingmanager.h"

//---------------------------------------------------------------------------------------------------------------------

void setupRoutes(server::HttpServer& server, server::PairingManager& pairing_manager,
                 os::PcControlInterface& pc_control);
