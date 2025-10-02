#ifndef FW_CONFIG_H
#define FW_CONFIG_H

// EEPROM Configuration
#define EEPROM_I2C_ADDRESS 0x50
#define EEPROM_SIZE 32768        // 32KB EEPROM
#define EEPROM_PAGE_SIZE 64      // Page size for writes

// I2C Pin Configuration
#define SDA_PIN 5                // GPIO4 (D2)
#define SCL_PIN 4                // GPIO5 (D1)

// Write-Protect Pin Configuration
#define EEPROM_WP_PIN 14         // GPIO14 (D5)
#define EEPROM_WP_ACTIVE_HIGH 1  // Set to 1 if WP is active HIGH

// Debug and Logging
#define EEPROM_DEBUG 0           // Set to 1 for verbose logging, 0 for production

// Chunk-based processing configuration
constexpr uint16_t EEPROM_WRITE_CHUNK_SIZE = 32;               // Optimal chunk size
constexpr uint16_t EEPROM_VERIFICATION_CHUNK_SIZE = 32;        // Verify in same chunk size  
constexpr uint8_t EEPROM_MAX_VERIFICATION_ERRORS_LOG = 3;      // Only log first few errors
// Add to config.h
#define MAX_VERIFICATION_CHUNK 512        // Maximum bytes to verify in one request
#define VERIFICATION_CHUNK_SIZE 256       // Default chunk size for verification
// Verification Settings
#define VERIFY_AFTER_WRITE 0     // Disabled by default for speed

// DSP (ADAU1701) Configuration
#define DSP_I2C_ADDRESS 0x34     // 8-bit write address for 7-bit 0x34
#define DSP_CONTROL_REGISTER 0xF000
#define DSP_RUN_BIT_MASK 0x01

// Memory Limits for ESP8266
#define MAX_HEAP_REQUIRED 4096   // Reserve 4KB for system stability
#define CONFIG_BUFFER_SIZE 512   // Buffer size for streaming

// WiFi Mode Configuration
#define WIFI_MODE_STA 1          // 1 for Station mode, 0 for AP mode

// Station Mode Credentials (only used if WIFI_MODE_STA = 1)
#define WIFI_STA_SSID "nevada"
#define WIFI_STA_PASS "Joska1948"

// Performance Settings
#define I2C_MAX_DATA_PER_XFER 30 // Max bytes per I2C transfer
#define MAX_UPLOAD_TIME_MS 120000 // 2 minute upload timeout

#endif