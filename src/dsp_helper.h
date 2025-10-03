#ifndef DSP_HELPER_H
#define DSP_HELPER_H

#include <Arduino.h>
#include <Wire.h>

// ADAU1701 Register Addresses (16-bit)
#define ADAU1701_REG_CONTROL         0xF000  // Core control register
#define ADAU1701_REG_STATUS          0xF001  // Core status register
#define ADAU1701_REG_HW_ID           0xF002  // Hardware ID (should be 0x02)
#define ADAU1701_REG_SW_ID           0xF003  // Software ID
#define ADAU1701_REG_SAFECLOCK       0xF004  // Safeload clock divider
#define ADAU1701_REG_SAFEDATA0       0xF010  // Safeload data register 0
#define ADAU1701_REG_SAFEDATA1       0xF011  // Safeload data register 1
#define ADAU1701_REG_SAFEDATA2       0xF012  // Safeload data register 2
#define ADAU1701_REG_SAFEDATA3       0xF013  // Safeload data register 3
#define ADAU1701_REG_SAFEDATA4       0xF014  // Safeload data register 4
#define ADAU1701_REG_SAFELOAD        0xF015  // Safeload trigger register

// ADAU1701 Control Register Bits
#define ADAU1701_CR_RUN              0x01    // Core run (0=running, 1=reset/stopped)
#define ADAU1701_CR_SAFEMODE         0x02    // Safeload mode
#define ADAU1701_CR_SRAM_EN          0x04    // SRAM enable
#define ADAU1701_CR_MUTE             0x08    // Mute outputs

// ADAU1701 Status Register Bits
#define ADAU1701_SR_DCK_STABLE       0x01    // DCK stable
#define ADAU1701_SR_RESET_ACTIVE     0x02    // Reset active
#define ADAU1701_SR_PLL_LOCKED       0x04    // PLL locked
#define ADAU1701_SR_SAFELOAD_RDY     0x08    // Safeload ready

// DSP Communication Configuration
#define DSP_I2C_TIMEOUT_MS          100     // I2C timeout
#define DSP_WRITE_VERIFY_RETRIES    3       // Number of write verification retries
#define DSP_RESET_DELAY_MS          50      // Delay after reset operations
#define DSP_COMM_DELAY_US           10      // Microsecond delay between operations

// DSP State Structure
struct DSPStatus {
    bool detected;
    uint8_t hardwareId;
    uint8_t softwareId;
    uint8_t controlReg;
    uint8_t statusReg;
    bool coreRunning;
    bool pllLocked;
    bool dckStable;
    bool safeloadReady;
};

// Core DSP Functions
bool dsp_init();
bool dsp_detect();
bool dsp_writeRegister(uint16_t regAddr, uint8_t value);
bool dsp_readRegister(uint16_t regAddr, uint8_t& value);
bool dsp_writeRegisterVerified(uint16_t regAddr, uint8_t value);
bool dsp_readMultipleRegisters(uint16_t startAddr, uint8_t* buffer, size_t count);

// High-level DSP Control Functions
bool dsp_setRunState(bool run);
bool dsp_softReset();
bool dsp_hardReset();
bool dsp_selfBoot();
DSPStatus dsp_getStatus();

// Safeload Functions (for parameter RAM updates)
bool dsp_safeloadWrite(uint16_t paramAddr, uint8_t* data, size_t count);
bool dsp_safeloadTrigger();

// Diagnostic Functions
void dsp_printStatus();
bool dsp_testCommunication();
String dsp_getErrorString(int errorCode);

// Legacy compatibility functions
bool setDSPRunState(bool run);
void resetDSP();
void readADAU1701Status();

#endif // DSP_HELPER_H
