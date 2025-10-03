#include "system_routes.h"
#include "eeprom_manager.h"
#include <ArduinoJson.h>
#include <WebServer.h>

// Global server reference (shared with other modules)
extern WebServer *g_server;

// Helper functions (shared with other modules)
extern void sendJson(int code, const JsonDocument& doc);

void handleHeap() {
  JsonDocument doc;
  doc["free_heap"] = ESP.getFreeHeap();
  sendJson(200, doc);
}

void handleI2CScan() {
  JsonDocument doc;
  doc["found"] = i2c_scan();
  sendJson(200, doc);
}

void handleI2CLog() {
  JsonDocument doc;
  doc["log"] = i2c_log_get_all();
  sendJson(200, doc);
}

void handleProgress() {
  JsonDocument doc;
  doc["bytesWritten"] = getBytesWrittenProgress();
  doc["bytesTotal"] = getBytesToWrite();
  doc["inProgress"] = isWriteInProgress();
  sendJson(200, doc);
}

void register_system_routes(WebServer &server) {
  // System operations
  server.on("/heap", HTTP_GET, handleHeap);
  server.on("/i2c_scan", HTTP_GET, handleI2CScan);
  server.on("/i2c_log", HTTP_GET, handleI2CLog);
  server.on("/progress", HTTP_GET, handleProgress);
}