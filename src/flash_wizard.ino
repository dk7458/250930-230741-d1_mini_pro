#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "config.h"
#include "html/index.html.h"  // Single-page HTML interface
#include "eeprom_manager.h"
#include "hex_parser.h"
#include "web_routes.h"

// WiFi settings
const char* ap_ssid = "EEPROM_Programmer";
const char* ap_password = "dupa1234";

WebServer server(80);

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C and EEPROM systems
  eeprom_begin();
  hex_begin();

  Serial.println("\n=== EEPROM Programmer Starting ===");
  Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());

  // WiFi Configuration
#if WIFI_MODE_STA_ENABLED
  // Station mode: connect to local WiFi
  Serial.println("WiFi Mode: STA (Station)");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASS);
  Serial.printf("Connecting to: %s\n", WIFI_STA_SSID);

  unsigned long start = millis();
  const unsigned long timeout = 15000;
  
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✓ Connected to WiFi");
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("✗ STA failed - falling back to AP mode");
    setupAPMode();
  }
#else
  // Access Point mode
  setupAPMode();
#endif

  // Register web routes and start server
  register_web_routes(server);
  server.begin();
  
  Serial.println("✓ HTTP server started");
  Serial.println("✓ Ready for EEPROM programming");
  Serial.println("===================================");
}

void setupAPMode() {
  Serial.println("WiFi Mode: AP (Access Point)");
  WiFi.mode(WIFI_AP);
  
  // Configure AP with better settings
  WiFi.softAPConfig(
    IPAddress(192, 168, 4, 1),
    IPAddress(192, 168, 4, 1), 
    IPAddress(255, 255, 255, 0)
  );
  
  bool apSuccess = WiFi.softAP(ap_ssid, ap_password);
  
  if (apSuccess) {
    Serial.println("✓ Access Point started");
    Serial.printf("SSID: %s\n", ap_ssid);
    Serial.printf("IP Address: %s\n", WiFi.softAPIP().toString().c_str());
  } else {
    Serial.println("✗ Failed to start Access Point");
  }
}

void loop() {
  server.handleClient();
  
  // Optional: Add periodic status reporting
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > 30000) { // Every 30 seconds
    lastStatus = millis();
    Serial.printf("Status: Heap=%d, Clients=%d\n", 
                 ESP.getFreeHeap(), WiFi.softAPgetStationNum());
  }
  
  // Small delay to prevent watchdog issues
  // ESP32 needs a larger delay to prevent watchdog resets
  delay(10);
}