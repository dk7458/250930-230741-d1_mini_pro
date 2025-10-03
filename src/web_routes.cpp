#include "web_routes.h"
#include "eeprom_routes.h"
#include "dsp_routes.h"
#include "upload_routes.h"
#include "system_routes.h"
#include "config.h"
#include "eeprom_manager.h"
#include "hex_parser.h"
#include "html/index.html.h"
#include <ArduinoJson.h>
#include <WebServer.h>
#include <Wire.h>

// Global server reference (shared across all modules)
WebServer *g_server = nullptr;

// Helper functions for file type detection (shared with upload module)
bool isHexFile(const String& filename) {
  return filename.endsWith(".hex") || filename.endsWith(".txt");
}

bool isBinFile(const String& filename) {
  return filename.endsWith(".bin") || filename.endsWith(".rom");
}

void sendJson(int code, const JsonDocument& doc) {
  String response;
  serializeJson(doc, response);
  g_server->send(code, "application/json", response);
}

bool checkMemorySafety() {
  if (ESP.getFreeHeap() < 2000) {
    Serial.printf("CRITICAL: Low memory - %d bytes free\n", ESP.getFreeHeap());
    return false;
  }
  return true;
}

// Simple route handlers
void handleRoot() {
  Serial.println("Serving main page");
  g_server->send(200, "text/html", MAIN_PAGE);
}

void handleWriteProtect() {
  Serial.println("Write protect");

  JsonDocument doc;
  if (!g_server->hasArg("plain")) {
    doc["success"] = false;
    doc["message"] = "No data";
    sendJson(400, doc);
    return;
  }

  String body = g_server->arg("plain");
  JsonDocument req;
  DeserializationError err = deserializeJson(req, body);

  if (err) {
    doc["success"] = false;
    doc["message"] = "Invalid JSON";
    sendJson(400, doc);
    return;
  }

  bool enable = req["enable"];
  setWriteProtect(enable);

  doc["success"] = true;
  doc["enabled"] = enable;
  sendJson(200, doc);
}

void handleVerification() {
  if (g_server->method() == HTTP_GET) {
    JsonDocument doc;
    doc["enabled"] = getVerificationEnabled();
    sendJson(200, doc);
  } else if (g_server->method() == HTTP_POST) {
    JsonDocument doc;

    if (!g_server->hasArg("plain")) {
      doc["success"] = false;
      doc["message"] = "No data";
      sendJson(400, doc);
      return;
    }

    String body = g_server->arg("plain");
    JsonDocument req;
    DeserializationError err = deserializeJson(req, body);

    if (err) {
      doc["success"] = false;
      doc["message"] = "Invalid JSON";
    } else {
      bool enabled = req["enabled"];
      setVerificationEnabled(enabled);
      doc["success"] = true;
      doc["enabled"] = enabled;
    }
    sendJson(200, doc);
  }
}

void handleTestWrite() {
  Serial.println("Test write");

  JsonDocument doc;
  if (!g_server->hasArg("plain")) {
    doc["success"] = false;
    doc["message"] = "No data";
    sendJson(400, doc);
    return;
  }

  String body = g_server->arg("plain");
  JsonDocument req;
  DeserializationError err = deserializeJson(req, body);

  if (err) {
    doc["success"] = false;
    doc["message"] = "Invalid JSON";
    sendJson(400, doc);
    return;
  }

  uint32_t addr = req["address"];
  uint32_t val = req["value"];
  bool ok = testWriteByte(addr, val);

  doc["success"] = ok;
  doc["address"] = addr;
  doc["value"] = val;
  sendJson(200, doc);
}

void register_web_routes(WebServer &server) {
  g_server = &server;

  // Main page
  server.on("/", HTTP_GET, handleRoot);

  // Register routes from all modules
  register_eeprom_routes(server);
  register_dsp_routes(server);
  register_upload_routes(server);
  register_system_routes(server);

  // Control operations (remaining in main module)
  server.on("/wp", HTTP_POST, handleWriteProtect);
  server.on("/verification", HTTP_GET, handleVerification);
  server.on("/verification", HTTP_POST, handleVerification);
  server.on("/test_write", HTTP_POST, handleTestWrite);

  Serial.println("All routes registered - Enhanced DSP control available");
}