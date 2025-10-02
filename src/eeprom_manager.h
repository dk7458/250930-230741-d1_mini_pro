#ifndef EEPROM_MANAGER_H
#define EEPROM_MANAGER_H

#include <Arduino.h>

// I2C Configuration
#define I2C_MAX_DATA_PER_XFER 30

// Core EEPROM Functions
void eeprom_begin();
bool checkEEPROM();
bool eraseEEPROM();
bool writeToEEPROM(uint16_t address, uint8_t data[], int length);
uint8_t readFromEEPROM(uint16_t address);

// Write Protection Control
void setWriteProtect(bool enable);

// DSP Control Functions
bool setDSPRunState(bool run);

// Progress Tracking
uint32_t getBytesWrittenProgress();
uint32_t getBytesToWrite();
bool isWriteInProgress();
void setExpectedTotalBytes(uint32_t total);
void resetWriteProgress();

// Verification Control
void setVerificationEnabled(bool enabled);
bool getVerificationEnabled();

// I2C Diagnostics
void i2c_log_add(const char* msg);
String i2c_log_get_all();
String i2c_scan();
bool testWriteByte(uint16_t address, uint8_t value);
// Add to eeprom_manager.h
bool verifyEEPROMRange(uint16_t startAddr, uint16_t length, uint8_t expectedData[]);
// Legacy function (kept for compatibility)
void resetDSP();

#endif