#ifndef WEB_ROUTES_H
#define WEB_ROUTES_H

#include <WebServer.h>

// Include all route module headers for backward compatibility
#include "eeprom_routes.h"
#include "dsp_routes.h"
#include "upload_routes.h"
#include "system_routes.h"

// Register all web routes with the server
void register_web_routes(WebServer &server);

// Legacy function declarations for backward compatibility
// (now implemented in respective modules)
void handleReadRange();
void handleVerifyRange();
void handleStressTest();
void handleDSPCoreRun();
void handleDSPSoftReset();
void handleDSPSelfBoot();
void handleDSPI2CDebug();
void handleDSPStatus();
void handleDSPRegisters();

#endif