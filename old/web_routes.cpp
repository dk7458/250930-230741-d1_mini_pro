#include "web_routes.h"
#include "config.h"
#include "eeprom_manager.h"
#include "index.html.h"
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>

// Static variables for upload handling
static ESP8266WebServer *g_server = nullptr;

// Forward declarations
void sendJson(ESP8266WebServer &server, const JsonDocument &doc);
void handleHeapInfo(ESP8266WebServer &server);

// Individual route handler declarations
void handleDetect(ESP8266WebServer &server);
void handleErase(ESP8266WebServer &server);
void handleUpload(ESP8266WebServer &server);
void handleTestWrite(ESP8266WebServer &server);
void handleWriteProtect(ESP8266WebServer &server);
void handleDSPRun(ESP8266WebServer &server);
void handleVerification(ESP8266WebServer &server);
void handleRead(ESP8266WebServer &server);
void handleDump(ESP8266WebServer &server);
void handleDebugUpload(ESP8266WebServer &server);

/**
 * Send JSON response to client
 */
void sendJson(ESP8266WebServer &server, const JsonDocument &doc) {
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

/**
 * Handle heap information request
 */
void handleHeapInfo(ESP8266WebServer &server) {
  JsonDocument doc;
  doc["free_heap"] = ESP.getFreeHeap();
  doc["max_alloc_heap"] = ESP.getMaxFreeBlockSize();
  doc["heap_fragmentation"] = ESP.getHeapFragmentation();
  sendJson(server, doc);
}

/**
 * Parse JSON body from request with error handling
 */
bool parseJsonBody(ESP8266WebServer &server, JsonDocument &doc, String &errorMsg) {
  if (!server.hasArg("plain")) {
    errorMsg = "No request body";
    return false;
  }
  
  String body = server.arg("plain");
  DeserializationError err = deserializeJson(doc, body);
  
  if (err) {
    errorMsg = "Invalid JSON: " + String(err.c_str());
    return false;
  }
  
  return true;
}

/**
 * Handle EEPROM detection
 */
void handleDetect(ESP8266WebServer &server) {
  JsonDocument doc;
  if (checkEEPROM()) {
    doc["success"] = true;
    doc["message"] = "EEPROM detected at address 0x50";
  } else {
    doc["success"] = false;
    doc["message"] = "EEPROM not found. Check connections.";
  }
  sendJson(server, doc);
}

/**
 * Handle EEPROM erase
 */
void handleErase(ESP8266WebServer &server) {
  JsonDocument doc;
  if (eraseEEPROM()) {
    doc["success"] = true;
    doc["message"] = "EEPROM erased successfully";
  } else {
    doc["success"] = false;
    doc["message"] = "EEPROM erase failed";
  }
  sendJson(server, doc);
}

/**
 * Handle direct (legacy) HEX upload — optional
 * Recommend replacing with BIN streaming
 */
void handleUpload(ESP8266WebServer &server) {
  JsonDocument doc;
  doc["success"] = false;
  doc["message"] = "Legacy HEX upload disabled. Use /upload_bin instead.";
  sendJson(server, doc);
}

/**
 * Handle test write operation
 */
void handleTestWrite(ESP8266WebServer &server) {
  JsonDocument doc;
  JsonDocument requestDoc;
  String errorMsg;
  
  if (!parseJsonBody(server, requestDoc, errorMsg)) {
    doc["success"] = false;
    doc["message"] = errorMsg;
    sendJson(server, doc);
    return;
  }

  if (!requestDoc["address"].is<uint32_t>() || !requestDoc["value"].is<uint32_t>()) {
    doc["success"] = false;
    doc["message"] = "Invalid address or value format";
    sendJson(server, doc);
    return;
  }

  uint32_t addr = requestDoc["address"].as<uint32_t>();
  uint8_t val = static_cast<uint8_t>(requestDoc["value"].as<uint32_t>());
  bool ok = testWriteByte(static_cast<uint16_t>(addr), val);
  
  doc["success"] = ok;
  doc["address"] = addr;
  doc["value"] = val;
  sendJson(server, doc);
}

/**
 * Handle write protect control
 */
void handleWriteProtect(ESP8266WebServer &server) {
  JsonDocument doc;
  JsonDocument requestDoc;
  String errorMsg;
  
  if (!parseJsonBody(server, requestDoc, errorMsg)) {
    doc["success"] = false;
    doc["message"] = errorMsg;
    sendJson(server, doc);
    return;
  }

  if (!requestDoc["enable"].is<bool>()) {
    doc["success"] = false;
    doc["message"] = "Invalid JSON or missing 'enable'";
    sendJson(server, doc);
    return;
  }

  bool enable = requestDoc["enable"].as<bool>();
  setWriteProtect(enable);
  doc["success"] = true;
  doc["enabled"] = enable;
  sendJson(server, doc);
}

/**
 * Handle DSP run state control
 */
void handleDSPRun(ESP8266WebServer &server) {
  JsonDocument doc;
  JsonDocument requestDoc;
  String errorMsg;
  
  if (!parseJsonBody(server, requestDoc, errorMsg)) {
    doc["success"] = false;
    doc["message"] = errorMsg;
    sendJson(server, doc);
    return;
  }

  if (!requestDoc["run"].is<bool>()) {
    doc["success"] = false;
    doc["message"] = "Invalid JSON or missing 'run'";
    sendJson(server, doc);
    return;
  }

  bool run = requestDoc["run"].as<bool>();
  bool ok = setDSPRunState(run);
  doc["success"] = ok;
  doc["run"] = run;
  sendJson(server, doc);
}

/**
 * Handle verification control
 */
void handleVerification(ESP8266WebServer &server) {
  JsonDocument doc;
  
  if (server.method() == HTTP_GET) {
    doc["enabled"] = getVerificationEnabled();
    sendJson(server, doc);
  } else if (server.method() == HTTP_POST) {
    JsonDocument requestDoc;
    String errorMsg;
    
    if (!parseJsonBody(server, requestDoc, errorMsg)) {
      doc["success"] = false;
      doc["message"] = errorMsg;
    } else if (!requestDoc["enabled"].is<bool>()) {
      doc["success"] = false;
      doc["message"] = "Invalid body";
    } else {
      bool enabled = requestDoc["enabled"].as<bool>();
      setVerificationEnabled(enabled);
      doc["success"] = true;
      doc["enabled"] = enabled;
    }
    sendJson(server, doc);
  }
}

/**
 * Handle EEPROM read (first 256 bytes)
 */
void handleRead(ESP8266WebServer &server) {
  JsonDocument doc;
  String data = "";
  
  for (uint16_t addr = 0; addr < 256; addr++) {
    uint8_t byteData = readFromEEPROM(addr);
    if (byteData < 16) data += "0";
    data += String(byteData, HEX) + " ";
    if ((addr + 1) % 16 == 0) data += "\n";
  }
  
  doc["success"] = true;
  doc["data"] = data;
  sendJson(server, doc);
}

/**
 * Handle EEPROM binary dump
 */
void handleDump(ESP8266WebServer &server) {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/octet-stream", "eeprom_dump.bin");
  
  const size_t BUFFER_SIZE = 64;
  uint8_t buffer[BUFFER_SIZE];
  
  for (uint16_t addr = 0; addr < EEPROM_SIZE; addr += BUFFER_SIZE) {
    for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
      buffer[i] = readFromEEPROM(addr + i);
    }
    server.sendContent(reinterpret_cast<const char*>(buffer), BUFFER_SIZE);
    if (addr % 1024 == 0) delay(1);
  }
}

/**
 * Debug endpoint for upload testing
 */
void handleDebugUpload(ESP8266WebServer &server) {
  Serial.println("=== DEBUG UPLOAD ===");
  Serial.printf("Headers: %d\n", server.headers());
  
  for (int i = 0; i < server.headers(); i++) {
    Serial.printf("  %s: %s\n", server.headerName(i).c_str(), server.header(i).c_str());
  }
  
  Serial.printf("Args: %d\n", server.args());
  for (int i = 0; i < server.args(); i++) {
    Serial.printf("  %s: %s\n", server.argName(i).c_str(), server.arg(i).c_str());
  }
  
  JsonDocument doc;
  doc["success"] = true;
  doc["message"] = "Debug received";
  doc["headers"] = server.headers();
  doc["args"] = server.args();
  sendJson(server, doc);
}

/**
 * Upload stream handler (binary mode)
 */
void handleUploadStream() {
  if (!g_server) return;
  HTTPUpload& upload = g_server->upload();

  switch (upload.status) {
    case UPLOAD_FILE_START:
      Serial.printf("\n=== BIN UPLOAD START: %s (%u bytes expected) ===\n",
                    upload.filename.c_str(), upload.totalSize);
      resetWriteProgress();
      setWriteProtect(false);
      setExpectedTotalBytes(upload.totalSize);
      break;

    case UPLOAD_FILE_WRITE:
      if (upload.currentSize > 0) {
        writeToEEPROM(getBytesWrittenProgress(), upload.buf, upload.currentSize);
        Serial.printf("EEPROM progress: %u/%u bytes\n",
                      getBytesWrittenProgress(), getBytesToWrite());
      }
      break;

    case UPLOAD_FILE_END:
      setWriteProtect(true);
      Serial.printf("=== BIN UPLOAD END: %u bytes written ===\n",
                    getBytesWrittenProgress());
      break;

    case UPLOAD_FILE_ABORTED:
      setWriteProtect(true);
      resetWriteProgress();
      Serial.println("!!! BIN UPLOAD ABORTED");
      break;
  }
}

/**
 * Upload stream completion handler (binary)
 */
void handleUploadStreamComplete() {
  if (!g_server) return;
  JsonDocument doc;

  uint32_t eepromBytes = getBytesWrittenProgress();
  uint32_t expected = getBytesToWrite();

  bool success = (eepromBytes > 0 && eepromBytes == expected);
  doc["success"] = success;
  doc["bytesWritten"] = eepromBytes;
  doc["bytesTotal"] = expected;
  doc["message"] = success ? "BIN file programmed successfully" : "Upload incomplete";

  String out;
  serializeJson(doc, out);
  g_server->send(200, "application/json", out);
}

/**
 * Register all web routes
 */
void register_web_routes(ESP8266WebServer &server) {
  g_server = &server;

  // Main page
  server.on("/", HTTP_GET, []() { 
    g_server->send(200, "text/html", MAIN_PAGE); 
  });

  // EEPROM operations
  server.on("/detect", HTTP_GET, []() { handleDetect(*g_server); });
  server.on("/erase", HTTP_POST, []() { handleErase(*g_server); });
  server.on("/verify", HTTP_GET, []() {
    JsonDocument doc;
    doc["success"] = checkEEPROM();
    doc["message"] = doc["success"] ? "EEPROM verification passed" : "EEPROM verification failed";
    sendJson(*g_server, doc);
  });
  server.on("/read", HTTP_GET, []() { handleRead(*g_server); });
  server.on("/dump", HTTP_GET, []() { handleDump(*g_server); });

  // Upload operations
  server.on("/upload", HTTP_POST, []() { handleUpload(*g_server); }); // legacy
  server.on("/upload_bin", HTTP_POST, handleUploadStreamComplete, handleUploadStream);
  server.on("/debug_upload", HTTP_POST, []() { handleDebugUpload(*g_server); });
  
  // Progress monitoring
  server.on("/progress", HTTP_GET, []() {
    JsonDocument doc;
    doc["bytesWritten"] = getBytesWrittenProgress();
    doc["bytesTotal"] = getBytesToWrite();
    doc["inProgress"] = isWriteInProgress();
    sendJson(*g_server, doc);
  });

  // I2C operations
  server.on("/i2c_log", HTTP_GET, []() {
    JsonDocument doc;
    doc["log"] = i2c_log_get_all();
    sendJson(*g_server, doc);
  });
  
  server.on("/i2c_scan", HTTP_GET, []() {
    JsonDocument doc;
    doc["found"] = i2c_scan();
    sendJson(*g_server, doc);
  });

  // Control operations
  server.on("/test_write", HTTP_POST, []() { handleTestWrite(*g_server); });
  server.on("/wp", HTTP_POST, []() { handleWriteProtect(*g_server); });
  server.on("/dsp_run", HTTP_POST, []() { handleDSPRun(*g_server); });
  server.on("/verification", HTTP_GET, []() { handleVerification(*g_server); });
  server.on("/verification", HTTP_POST, []() { handleVerification(*g_server); });

  // System info
  server.on("/heap", HTTP_GET, []() { handleHeapInfo(*g_server); });
}
