#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "config.h"
#include "eeprom_manager.h"
#include "hex_parser.h"
#include "index.html.h"

class WebServerManager {
private:
    static ESP8266WebServer server;

public:
    static void initialize() {
        server.on("/", HTTP_GET, handleRoot);
        server.on("/detect", HTTP_GET, handleDetectEEPROM);
        server.on("/erase", HTTP_POST, handleEraseEEPROM);
        server.on("/upload", HTTP_POST, handleUploadHex);
        server.on("/verify", HTTP_GET, handleVerifyEEPROM);
        server.on("/read", HTTP_GET, handleReadEEPROM);
        server.on("/dump", HTTP_GET, handleDumpEEPROM);
        
        server.begin();
        Serial.println("✅ HTTP server started on port " + String(SERVER_PORT));
    }
    
    static void handleClient() {
        server.handleClient();
    }

private:
    static void handleRoot() {
        server.send(200, "text/html; charset=utf-8", MAIN_PAGE);
    }
    
    static void handleDetectEEPROM() {
        Serial.println("🔍 EEPROM detection requested");
        
        DynamicJsonDocument doc(512);
        
        if (EEPROMManager::checkEEPROM()) {
            doc["success"] = true;
            doc["message"] = "EEPROM detected successfully at address 0x" + String(EEPROM_I2C_ADDRESS, HEX) + " (24LC256 compatible)";
            Serial.println("✅ EEPROM detected at 0x50");
        } else {
            doc["success"] = false;
            doc["message"] = "EEPROM not detected at address 0x50. Please check: 1) I2C connections (SDA=GPIO5, SCL=GPIO4) 2) Power 3) Pull-up resistors";
            Serial.println("❌ EEPROM not detected");
        }
        
        sendJsonResponse(doc);
    }
    
    static void handleEraseEEPROM() {
        Serial.println("🗑️ EEPROM erase requested");
        
        DynamicJsonDocument doc(512);
        
        if (EEPROMManager::eraseEEPROM()) {
            doc["success"] = true;
            doc["message"] = "EEPROM successfully erased (all bytes set to 0xFF)";
            Serial.println("✅ EEPROM erased successfully");
        } else {
            doc["success"] = false;
            doc["message"] = "EEPROM erase failed. Check I2C connection and try again.";
            Serial.println("❌ EEPROM erase failed");
        }
        
        sendJsonResponse(doc);
    }
    
    static void handleUploadHex() {
        Serial.println("📤 HEX upload requested");
        
        DynamicJsonDocument doc(512);
        
        if (server.hasArg("plain")) {
            String hexContent = server.arg("plain");
            Serial.println("Received HEX content, length: " + String(hexContent.length()));
            
            DynamicJsonDocument jsonDoc(hexContent.length() * 2);
            DeserializationError error = deserializeJson(jsonDoc, hexContent);
            
            if (error) {
                doc["success"] = false;
                doc["message"] = "Invalid JSON data received";
                Serial.println("❌ JSON parsing error: " + String(error.c_str()));
            } else {
                String hexData = jsonDoc["hex"].as<String>();
                int bytesWritten = HexParser::writeHexToEEPROM(hexData);
                
                if (bytesWritten > 0) {
                    doc["success"] = true;
                    doc["bytesWritten"] = bytesWritten;
                    doc["message"] = "HEX file processed and programmed successfully";
                    Serial.println("✅ HEX file processed, bytes written: " + String(bytesWritten));
                } else {
                    doc["success"] = false;
                    doc["message"] = "Failed to process HEX file. Check format and try again.";
                    Serial.println("❌ HEX file processing failed");
                }
            }
        } else {
            doc["success"] = false;
            doc["message"] = "No HEX data received in request";
            Serial.println("❌ No HEX data received");
        }
        
        sendJsonResponse(doc);
    }
    
    static void handleVerifyEEPROM() {
        Serial.println("✅ EEPROM verification requested");
        
        DynamicJsonDocument doc(512);
        
        if (EEPROMManager::advancedEEPROMCheck()) {
            doc["success"] = true;
            doc["message"] = "EEPROM verification passed! Device is responsive and functioning correctly.";
            Serial.println("✅ EEPROM verification passed");
        } else {
            doc["success"] = false;
            doc["message"] = "EEPROM verification failed. Device may be disconnected or malfunctioning.";
            Serial.println("❌ EEPROM verification failed");
        }
        
        sendJsonResponse(doc);
    }
    
    static void handleReadEEPROM() {
        Serial.println("📖 EEPROM read requested");
        
        DynamicJsonDocument doc(1024);
        
        String data = EEPROMManager::readEEPROMRange(0, 256);
        doc["success"] = true;
        doc["data"] = data;
        Serial.println("✅ EEPROM read completed");
        
        sendJsonResponse(doc);
    }
    
    static void handleDumpEEPROM() {
        Serial.println("💾 EEPROM dump requested");
        
        server.setContentLength(CONTENT_LENGTH_UNKNOWN);
        server.send(200, "application/octet-stream", "eeprom_dump.bin");
        
        uint8_t buffer[128];
        uint32_t totalBytes = 0;
        
        for (uint16_t addr = 0; addr < EEPROM_SIZE; addr += sizeof(buffer)) {
            for (uint8_t i = 0; i < sizeof(buffer) && (addr + i) < EEPROM_SIZE; i++) {
                buffer[i] = EEPROMManager::readFromEEPROM(addr + i);
            }
            server.sendContent_P((const char*)buffer, sizeof(buffer));
            totalBytes += sizeof(buffer);
            
            if (addr % 1024 == 0) {
                delay(1);
            }
        }
        
        Serial.println("✅ EEPROM dump completed, bytes sent: " + String(totalBytes));
    }
    
    static void sendJsonResponse(DynamicJsonDocument& doc) {
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json; charset=utf-8", response);
    }
};

// Static member definition
ESP8266WebServer WebServerManager::server(SERVER_PORT);

#endif
