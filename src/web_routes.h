#ifndef WEB_ROUTES_H
#define WEB_ROUTES_H

#include <ESP8266WebServer.h>

// Register all web routes with the server
void register_web_routes(ESP8266WebServer &server);

// Enhanced EEPROM operations
void handleReadRange();
void handleVerifyRange();
void handleStressTest();

// Enhanced DSP Control routes
void handleDSPCoreRun();
void handleDSPSoftReset();
void handleDSPSelfBoot();
void handleDSPI2CDebug();
void handleDSPStatus();
void handleDSPRegisters();

#endif