#include "dsp_helper.h"
#include "config.h"
#include <Wire.h>

// External dependency for EEPROM write protect (used in self-boot)
extern void setWriteProtect(bool enable);

// DSP Communication State
static bool g_dsp_initialized = false;
static uint8_t g_dsp_address = DSP_I2C_ADDRESS;
static int g_last_error = 0;

// Error codes
#define DSP_ERR_NONE              0
#define DSP_ERR_I2C_TIMEOUT      -1
#define DSP_ERR_I2C_NACK         -2
#define DSP_ERR_VERIFY_FAILED    -3
#define DSP_ERR_INVALID_PARAM    -4
#define DSP_ERR_NOT_DETECTED     -5

// Forward declarations for internal functions
static bool dsp_writeRegisterInternal(uint16_t regAddr, uint8_t value);
static bool dsp_readRegisterInternal(uint16_t regAddr, uint8_t& value);
static void dsp_setLastError(int error);

// Initialize DSP communication
bool dsp_init() {
    if (g_dsp_initialized) {
        return true;
    }

    // Test basic communication
    if (!dsp_detect()) {
        Serial.println("DSP: Device not detected at configured address");
        return false;
    }

    g_dsp_initialized = true;
    Serial.println("DSP: Initialized successfully");
    return true;
}

// Detect DSP device on I2C bus
bool dsp_detect() {
    Wire.beginTransmission(g_dsp_address);
    int result = Wire.endTransmission();

    if (result == 0) {
        Serial.printf("DSP: Detected at address 0x%02X\n", g_dsp_address);
        return true;
    }

    // Try alternative addresses (ADAU1701 can be configured for different addresses)
    uint8_t alt_addresses[] = {0x68, 0x34, 0x1D}; // Common ADAU1701 addresses

    for (uint8_t alt_addr : alt_addresses) {
        if (alt_addr == g_dsp_address) continue;

        Wire.beginTransmission(alt_addr);
        result = Wire.endTransmission();
        if (result == 0) {
            Serial.printf("DSP: Detected at alternative address 0x%02X (configured: 0x%02X)\n",
                         alt_addr, g_dsp_address);
            g_dsp_address = alt_addr; // Update to working address
            return true;
        }
    }

    dsp_setLastError(DSP_ERR_NOT_DETECTED);
    return false;
}

// Internal register write (no verification)
static bool dsp_writeRegisterInternal(uint16_t regAddr, uint8_t value) {
    Wire.beginTransmission(g_dsp_address);

    // ADAU1701 uses 16-bit register addresses, sent as two bytes
    Wire.write((uint8_t)(regAddr >> 8));    // High byte
    Wire.write((uint8_t)(regAddr & 0xFF));  // Low byte
    Wire.write(value);                      // Data byte

    int result = Wire.endTransmission();

    if (result != 0) {
        dsp_setLastError(result == 5 ? DSP_ERR_I2C_TIMEOUT : DSP_ERR_I2C_NACK);
        return false;
    }

    delayMicroseconds(DSP_COMM_DELAY_US); // Allow DSP to process
    return true;
}

// Internal register read
static bool dsp_readRegisterInternal(uint16_t regAddr, uint8_t& value) {
    // Write register address
    Wire.beginTransmission(g_dsp_address);
    Wire.write((uint8_t)(regAddr >> 8));    // High byte
    Wire.write((uint8_t)(regAddr & 0xFF));  // Low byte
    int writeResult = Wire.endTransmission(false); // Restart condition

    if (writeResult != 0) {
        dsp_setLastError(DSP_ERR_I2C_NACK);
        return false;
    }

    // Read data
    Wire.requestFrom(g_dsp_address, (uint8_t)1);
    unsigned long startTime = millis();

    while (!Wire.available()) {
        if (millis() - startTime > DSP_I2C_TIMEOUT_MS) {
            dsp_setLastError(DSP_ERR_I2C_TIMEOUT);
            return false;
        }
        delay(1);
    }

    value = Wire.read();
    delayMicroseconds(DSP_COMM_DELAY_US);
    return true;
}

// Public register write with verification
bool dsp_writeRegister(uint16_t regAddr, uint8_t value) {
    return dsp_writeRegisterInternal(regAddr, value);
}

// Public register read
bool dsp_readRegister(uint16_t regAddr, uint8_t& value) {
    return dsp_readRegisterInternal(regAddr, value);
}

// Write register with verification
bool dsp_writeRegisterVerified(uint16_t regAddr, uint8_t value) {
    for (int attempt = 0; attempt < DSP_WRITE_VERIFY_RETRIES; attempt++) {
        if (!dsp_writeRegisterInternal(regAddr, value)) {
            continue; // Retry on write failure
        }

        // Verify the write
        uint8_t readback;
        if (dsp_readRegisterInternal(regAddr, readback) && readback == value) {
            return true; // Success
        }

        delay(5); // Small delay before retry
    }

    dsp_setLastError(DSP_ERR_VERIFY_FAILED);
    return false;
}

// Read multiple consecutive registers
bool dsp_readMultipleRegisters(uint16_t startAddr, uint8_t* buffer, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (!dsp_readRegisterInternal(startAddr + i, buffer[i])) {
            return false;
        }
    }
    return true;
}

// Set DSP core run state
bool dsp_setRunState(bool run) {
    uint8_t controlValue = run ? 0x00 : ADAU1701_CR_RUN; // 0=running, 1=reset/stopped

    Serial.printf("DSP: Setting run state to %s\n", run ? "RUNNING" : "STOPPED");

    if (!dsp_writeRegisterVerified(ADAU1701_REG_CONTROL, controlValue)) {
        Serial.println("DSP: Failed to set run state");
        return false;
    }

    Serial.println("DSP: Run state set successfully");
    return true;
}

// Perform soft reset (toggle reset bit)
bool dsp_softReset() {
    Serial.println("DSP: Performing soft reset");

    // Set reset bit (stop core)
    if (!dsp_writeRegisterVerified(ADAU1701_REG_CONTROL, ADAU1701_CR_RUN)) {
        return false;
    }

    delay(DSP_RESET_DELAY_MS);

    // Clear reset bit (start core)
    if (!dsp_writeRegisterVerified(ADAU1701_REG_CONTROL, 0x00)) {
        return false;
    }

    delay(DSP_RESET_DELAY_MS);
    Serial.println("DSP: Soft reset completed");
    return true;
}

// Perform hard reset (requires hardware reset pin)
bool dsp_hardReset() {
    // Note: This would require a hardware reset pin connection
    // For now, fall back to soft reset
    Serial.println("DSP: Hard reset not implemented, using soft reset");
    return dsp_softReset();
}

// Trigger self-boot from EEPROM
bool dsp_selfBoot() {
    Serial.println("DSP: Triggering self-boot from EEPROM");

    // Ensure EEPROM write protect is disabled for self-boot
    setWriteProtect(true); // WP high for self-boot

    // Perform reset to trigger boot from EEPROM
    return dsp_softReset();
}

// Safeload write for parameter RAM (more reliable than direct writes)
bool dsp_safeloadWrite(uint16_t paramAddr, uint8_t* data, size_t count) {
    if (count > 5 || count == 0) {
        dsp_setLastError(DSP_ERR_INVALID_PARAM);
        return false;
    }

    // Wait for safeload ready
    uint8_t status;
    unsigned long startTime = millis();
    while (true) {
        if (!dsp_readRegister(ADAU1701_REG_STATUS, status)) {
            return false;
        }
        if (status & ADAU1701_SR_SAFELOAD_RDY) {
            break; // Ready
        }
        if (millis() - startTime > 1000) { // 1 second timeout
            Serial.println("DSP: Safeload timeout");
            return false;
        }
        delay(10);
    }

    // Write parameter address (big-endian)
    uint8_t addrHigh = (paramAddr >> 8) & 0xFF;
    uint8_t addrLow = paramAddr & 0xFF;

    if (!dsp_writeRegister(ADAU1701_REG_SAFEDATA0, addrHigh)) return false;
    if (!dsp_writeRegister(ADAU1701_REG_SAFEDATA1, addrLow)) return false;

    // Write data bytes
    for (size_t i = 0; i < count; i++) {
        if (!dsp_writeRegister(ADAU1701_REG_SAFEDATA2 + i, data[i])) return false;
    }

    // Pad with zeros if less than 5 bytes
    for (size_t i = count; i < 5; i++) {
        if (!dsp_writeRegister(ADAU1701_REG_SAFEDATA2 + i, 0x00)) return false;
    }

    return true;
}

// Trigger safeload operation
bool dsp_safeloadTrigger() {
    return dsp_writeRegister(ADAU1701_REG_SAFELOAD, 0x01);
}

// Get comprehensive DSP status
DSPStatus dsp_getStatus() {
    DSPStatus status = {false, 0, 0, 0, 0, false, false, false, false};

    status.detected = dsp_detect();

    if (!status.detected) {
        return status;
    }

    // Read registers
    dsp_readRegister(ADAU1701_REG_HW_ID, status.hardwareId);
    dsp_readRegister(ADAU1701_REG_SW_ID, status.softwareId);
    dsp_readRegister(ADAU1701_REG_CONTROL, status.controlReg);
    dsp_readRegister(ADAU1701_REG_STATUS, status.statusReg);

    // Interpret status bits
    status.coreRunning = !(status.controlReg & ADAU1701_CR_RUN);
    status.pllLocked = (status.statusReg & ADAU1701_SR_PLL_LOCKED) != 0;
    status.dckStable = (status.statusReg & ADAU1701_SR_DCK_STABLE) != 0;
    status.safeloadReady = (status.statusReg & ADAU1701_SR_SAFELOAD_RDY) != 0;

    return status;
}

// Print detailed DSP status
void dsp_printStatus() {
    DSPStatus status = dsp_getStatus();

    Serial.println("\n=== ADAU1701 Status ===");
    Serial.printf("Detected: %s\n", status.detected ? "YES" : "NO");

    if (!status.detected) {
        Serial.println("========================\n");
        return;
    }

    Serial.printf("I2C Address: 0x%02X\n", g_dsp_address);
    Serial.printf("Hardware ID: 0x%02X %s\n", status.hardwareId,
                 status.hardwareId == 0x02 ? "(ADAU1701)" : "(UNKNOWN)");
    Serial.printf("Software ID: 0x%02X\n", status.softwareId);
    Serial.printf("Control Reg: 0x%02X\n", status.controlReg);
    Serial.printf("  - Core State: %s\n", status.coreRunning ? "RUNNING" : "STOPPED/RESET");
    Serial.printf("Status Reg: 0x%02X\n", status.statusReg);
    Serial.printf("  - DCK Stable: %s\n", status.dckStable ? "YES" : "NO");
    Serial.printf("  - PLL Locked: %s\n", status.pllLocked ? "YES" : "NO");
    Serial.printf("  - Safeload Ready: %s\n", status.safeloadReady ? "YES" : "NO");
    Serial.println("========================\n");
}

// Test basic DSP communication
bool dsp_testCommunication() {
    uint8_t testValue = 0xAA;
    uint8_t readback;

    // Try to write and read back a test register (use a safe register)
    if (!dsp_writeRegister(ADAU1701_REG_SAFECLOCK, testValue)) {
        return false;
    }

    if (!dsp_readRegister(ADAU1701_REG_SAFECLOCK, readback)) {
        return false;
    }

    // Restore original value (we don't care what it was)
    dsp_writeRegister(ADAU1701_REG_SAFECLOCK, 0x00);

    return (readback == testValue);
}

// Get error string for error code
String dsp_getErrorString(int errorCode) {
    switch (errorCode) {
        case DSP_ERR_NONE: return "No error";
        case DSP_ERR_I2C_TIMEOUT: return "I2C timeout";
        case DSP_ERR_I2C_NACK: return "I2C NACK received";
        case DSP_ERR_VERIFY_FAILED: return "Write verification failed";
        case DSP_ERR_INVALID_PARAM: return "Invalid parameter";
        case DSP_ERR_NOT_DETECTED: return "DSP not detected";
        default: return "Unknown error";
    }
}

// Set last error code
static void dsp_setLastError(int error) {
    g_last_error = error;
}

// Legacy compatibility functions
bool setDSPRunState(bool run) {
    return dsp_setRunState(run);
}

void resetDSP() {
    dsp_softReset();
}

void readADAU1701Status() {
    dsp_printStatus();
}
