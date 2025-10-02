#ifndef EEPROM_MANAGER_H
#define EEPROM_MANAGER_H
#define I2C_MAX_DATA_PER_XFER 30


#include <Arduino.h>

void eeprom_begin();
bool checkEEPROM();
bool eraseEEPROM();
bool writeToEEPROM(uint16_t address, uint8_t data[], int length);
uint8_t readFromEEPROM(uint16_t address);

// Optional reset helper (kept for compatibility)
void resetDSP();

// New helpers
// Control write-protect pin: true to enable WP, false to disable
void setWriteProtect(bool enable);

// Control DSP core run/boot bit via I2C (per datasheet)
bool setDSPRunState(bool run);

// Progress helpers for streaming UI
uint32_t getBytesWrittenProgress();
uint32_t getBytesToWrite();
bool isWriteInProgress();
// Allow setting expected total bytes (optional, used to improve UI progress)
void setExpectedTotalBytes(uint32_t total);
// Reset internal progress counters
void resetWriteProgress();
// I2C log helpers
void i2c_log_add(const char* msg);
String i2c_log_get_all();

// Diagnostics helpers
// Scan I2C bus and return bitmask/list of addresses present
String i2c_scan();
// Test single-byte write/read to EEPROM at address; returns true on success
bool testWriteByte(uint16_t address, uint8_t value);

// Runtime control for verification behavior (can be toggled via web UI)
void setVerificationEnabled(bool enabled);
bool getVerificationEnabled();

#endif
