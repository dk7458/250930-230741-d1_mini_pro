#ifndef SYSTEM_ROUTES_H
#define SYSTEM_ROUTES_H

#include <WebServer.h>

// System route handlers
void handleHeap();
void handleI2CScan();
void handleI2CLog();
void handleProgress();

// System routes registration
void register_system_routes(WebServer &server);

#endif // SYSTEM_ROUTES_H