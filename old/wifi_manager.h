#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <ESP8266WiFi.h>
#include <Wire.h>
#include "config.h"

class WiFiManager {
public:
    static void initialize() {
        Serial.begin(SERIAL_BAUD_RATE);
        
        // Initialize I2C with custom pins
        Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
        
        // Set up Soft AP
        WiFi.mode(WIFI_AP);
        bool success = WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
        
        if (success) {
            Serial.println("\n🚀 EEPROM Programmer Started!");
            Serial.println("📡 Access Point Details:");
            Serial.print("   SSID: "); Serial.println(WIFI_SSID);
            Serial.print("   Password: "); Serial.println(WIFI_PASSWORD);
            Serial.print("   IP Address: "); Serial.println(WiFi.softAPIP());
            Serial.println("🔧 I2C Configuration:");
            Serial.print("   SDA: GPIO "); Serial.println(I2C_SDA_PIN);
            Serial.print("   SCL: GPIO "); Serial.println(I2C_SCL_PIN);
            Serial.print("   Address: 0x"); Serial.println(EEPROM_I2C_ADDRESS, HEX);
        } else {
            Serial.println("❌ Failed to start Access Point!");
        }
    }
    
    static String getIPAddress() {
        return WiFi.softAPIP().toString();
    }
};

#endif
