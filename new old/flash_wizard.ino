#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "config.h"
#include "index.html.h"  // Include HTML page as a header file
#include "eeprom_manager.h"
#include "hex_parser.h"
#include "web_routes.h"

// WiFi Soft AP settings (defaults)
const char* ap_ssid = "EEPROM_Programmer";
const char* ap_password = "dupa1234";

ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);
  // Initialize I2C and parser state
  eeprom_begin();
  hex_begin();

  Serial.println("\nEEPROM Programmer Starting...");

#if WIFI_MODE_STA
  // Station mode: attempt to connect to local WiFi
  Serial.println("Config: WIFI_MODE_STA = 1 -> starting in STA mode");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASS);
  Serial.print("Connecting to STA network "); Serial.print(WIFI_STA_SSID); Serial.println("...");

  unsigned long start = millis();
  const unsigned long timeout = 15000; // 15s
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi (STA)");
    Serial.print("IP Address: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("Failed to connect in STA mode within timeout — falling back to AP mode");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid, ap_password);
    Serial.print("AP IP Address: "); Serial.println(WiFi.softAPIP());
  }
#else
  // Soft AP mode
  Serial.println("Config: WIFI_MODE_STA = 0 -> starting in SoftAP mode");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("AP IP Address: "); Serial.println(WiFi.softAPIP());
#endif

  Serial.print("Free Heap: ");
  Serial.println(ESP.getFreeHeap());

  // Register routes implemented in web_routes.cpp
  register_web_routes(server);
  server.begin();
  Serial.println("HTTP server started with streaming support");
}

void loop() {
  server.handleClient();
}

// All peripheral and upload/parse functionality is implemented in modules:
// - eeprom_manager.{h,cpp}
// - hex_parser.{h,cpp}
// - web_routes.{h,cpp}

// If you need to expose helper wrappers here, do so as thin calls to the
// module functions (e.g., resetDSP(), readFromEEPROM(), handleHeapInfo are
// provided by eeprom_manager and web_routes respectively).
