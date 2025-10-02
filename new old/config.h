#ifndef FW_CONFIG_H
#define FW_CONFIG_H

// EEPROM settings
#define EEPROM_I2C_ADDRESS 0x50
#define EEPROM_SIZE 32768
#define EEPROM_PAGE_SIZE 64

// I2C Pins (SDA, SCL) - common wiring is SDA=GPIO4 (D2), SCL=GPIO5 (D1)
// Previous code used these reversed which can cause NACKs when the wiring expects the common mapping.
#define SDA_PIN 5
#define SCL_PIN 4

// Write-protect pin for EEPROM (GPIO14 / D5 on many ESP8266 boards)
#define EEPROM_WP_PIN 14
// If WP is active HIGH (true) then driving the pin HIGH asserts write-protect.
// If uncertain about the hardware, set to 1 (common) and test; you can invert if needed.
#define EEPROM_WP_ACTIVE_HIGH 1

// Verification during write (byte-by-byte read-back). Set to 1 to enable.
#define VERIFY_AFTER_WRITE 1

// DSP (ADAU1701) I2C control defaults. These are assumptions — adjust if your hardware uses
// a different 7-bit address or register map. See README/checklist for assumptions made.
#define DSP_I2C_ADDRESS 0x34
#define DSP_CONTROL_REGISTER 0x00
#define DSP_RUN_BIT_MASK 0x01

// Memory limits
#define MAX_HEAP_REQUIRED 4096  // Reserve 4KB for system
#define CONFIG_BUFFER_SIZE 512         // Small buffer for streaming

// WiFi mode toggle: set to 1 for STA (station mode), 0 for AP (softAP)
// If STA is selected, set WIFI_STA_SSID and WIFI_STA_PASS below.
#define WIFI_MODE_STA 1
#define WIFI_STA_SSID "nevada"
#define WIFI_STA_PASS "Joska1948"

#endif
