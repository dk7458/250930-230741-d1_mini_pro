#ifndef EEPROM_ROUTES_H
#define EEPROM_ROUTES_H

#include <WebServer.h>

// EEPROM route handlers
void handleDetect();
void handleErase();
void handleRead();
void handleDump();
void handleReadRange();
void handleVerifyRange();
void handleStressTest();

// EEPROM routes registration
void register_eeprom_routes(WebServer &server);

#endif // EEPROM_ROUTES_H