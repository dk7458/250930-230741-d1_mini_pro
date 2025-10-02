#ifndef CONFIG_H
#define CONFIG_H

// Arduino Libraries
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

// WiFi Configuration
#define WIFI_SSID "EEPROM_Programmer"
#define WIFI_PASSWORD "dupa1234"

// EEPROM Configuration
#define EEPROM_I2C_ADDRESS 0x50
#define EEPROM_SIZE 32768  // 32KB for 24LC256
#define EEPROM_PAGE_SIZE 64

// I2C Pin Configuration
#define I2C_SDA_PIN 5
#define I2C_SCL_PIN 4

// Server Configuration
#define SERVER_PORT 80

// Debug Configuration
#define SERIAL_BAUD_RATE 115200

#endif
