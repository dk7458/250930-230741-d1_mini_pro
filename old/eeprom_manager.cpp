// Rewritten clean implementation
#include <Wire.h>
#include "eeprom_manager.h"
#include "config.h"
#include <Arduino.h>

// Max data per I2C transaction (ESP8266 Wire buffer = 32, minus 2 for address)
#define I2C_MAX_DATA_PER_XFER 30

// Simple I2C log (ring buffer of strings)
static const int I2C_LOG_ENTRIES = 64;
static String i2c_log[I2C_LOG_ENTRIES];
static int i2c_log_idx = 0;

void i2c_log_add(const char* msg) {
  i2c_log[i2c_log_idx] = String(msg);
  i2c_log_idx = (i2c_log_idx + 1) % I2C_LOG_ENTRIES;
}

String i2c_log_get_all() {
  String out = "";
  for (int i = 0; i < I2C_LOG_ENTRIES; i++) {
    int idx = (i2c_log_idx + i) % I2C_LOG_ENTRIES;
    if (i2c_log[idx].length() > 0) {
      out += i2c_log[idx];
      out += '\n';
    }
  }
  return out;
}

String i2c_scan() {
  String out = "";
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    int res = Wire.endTransmission();
    if (res == 0) {
      char buf[32];
      snprintf(buf, sizeof(buf), "0x%02X ", addr);
      out += String(buf);
    }
    // Yield to allow background WiFi/HTTP processing and keep watchdog happy
    yield();
  }
  if (out.length() == 0) return String("none");
  return out;
}

bool testWriteByte(uint16_t address, uint8_t value) {
  uint8_t pageBuf[1];
  pageBuf[0] = value;
  setWriteProtect(false);
  bool ok = writeToEEPROM(address, pageBuf, 1);
  if (!ok) {
    setWriteProtect(true);
    return false;
  }
  uint8_t actual = readFromEEPROM(address);
  setWriteProtect(true);
  return (actual == value);
}

// Module-level state for progress reporting and verification
static volatile uint32_t g_bytes_written = 0;
static volatile uint32_t g_total_bytes_to_write = 0;
static volatile bool g_write_in_progress = false;
// runtime toggle for verification (default from compile-time macro)
#if defined(VERIFY_AFTER_WRITE)
static volatile bool g_verify_after_write = (VERIFY_AFTER_WRITE != 0);
#else
static volatile bool g_verify_after_write = false; // CHANGED: Default to false for speed
#endif

void eeprom_begin() {
  Wire.begin(SDA_PIN, SCL_PIN);
  pinMode(EEPROM_WP_PIN, OUTPUT);
  if (EEPROM_WP_ACTIVE_HIGH) digitalWrite(EEPROM_WP_PIN, LOW); 
  else digitalWrite(EEPROM_WP_PIN, HIGH);
}


bool checkEEPROM() {
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);
  return (Wire.endTransmission() == 0);
}

bool eraseEEPROM() {
  uint8_t blankData[EEPROM_PAGE_SIZE];
  memset(blankData, 0xFF, EEPROM_PAGE_SIZE);
  setWriteProtect(false);
  bool ok = true;
  
  // Reset progress tracking for erase
  g_bytes_written = 0;
  g_total_bytes_to_write = EEPROM_SIZE;
  g_write_in_progress = true;
  
  for (uint16_t addr = 0; addr < EEPROM_SIZE; addr += EEPROM_PAGE_SIZE) {
    if (!writeToEEPROM(addr, blankData, EEPROM_PAGE_SIZE)) { 
      ok = false; 
      break; 
    }
    // Update progress
    g_bytes_written += EEPROM_PAGE_SIZE;
    
    // yield() within writes will keep network stack responsive; still give a small pause
    yield();
    delay(1);
  }
  
  g_write_in_progress = false;
  setWriteProtect(true);
  return ok;
}

bool writeToEEPROM(uint16_t address, uint8_t data[], int length) {
  // CRITICAL FIX: Check if we're about to exceed EEPROM size
  if (address + length > EEPROM_SIZE) {
    Serial.printf("ERROR: Write would exceed EEPROM size! address=%d, length=%d, max=%d\n", 
                 address, length, EEPROM_SIZE);
    return false;
  }
  
  Serial.print("writeToEEPROM() called address="); 
  Serial.print(address); 
  Serial.print(" length="); 
  Serial.println(length);
  // CRITICAL FIX: Don't reset g_bytes_written here - it accumulates across multiple calls
  g_write_in_progress = true;
  setWriteProtect(false);

  int bytesWritten = 0;


while (bytesWritten < length) {
    int pageOffset = (address + bytesWritten) % EEPROM_PAGE_SIZE;
    int pageRemaining = EEPROM_PAGE_SIZE - pageOffset;

    int maxForThisXfer = min(pageRemaining, (int)I2C_MAX_DATA_PER_XFER);
    int bytesToWrite = min(maxForThisXfer, (int)(length - bytesWritten));

    // begin transmission (send 2-byte EEPROM address)
    Wire.beginTransmission(EEPROM_I2C_ADDRESS);
    Wire.write((uint8_t)((address + bytesWritten) >> 8));
    Wire.write((uint8_t)((address + bytesWritten) & 0xFF));
    Wire.write(data + bytesWritten, bytesToWrite);

    int res = -1;
    const int MAX_I2C_RETRIES = 3;
    for (int attempt = 0; attempt < MAX_I2C_RETRIES; attempt++) {
        res = Wire.endTransmission();
        if (res == 0) break;
        char buf[128];
        snprintf(buf, sizeof(buf), "I2C write attempt %d fail at addr %u res=%d", attempt + 1, address + bytesWritten, res);
        Serial.println(buf);
        i2c_log_add(buf);
        delay(5);
        yield();
    }
    if (res != 0) {
        char buf[128];
        snprintf(buf, sizeof(buf), "I2C write error at addr %u final res=%d", address + bytesWritten, res);
        Serial.println(buf);
        i2c_log_add(buf);
        g_write_in_progress = false;
        setWriteProtect(true);
        Serial.println("writeToEEPROM(): write failed (I2C)");
        return false;
    }

    // poll for ACK (write cycle completion)
    unsigned long start = millis();
    while (millis() - start < 50) {
        Wire.beginTransmission(EEPROM_I2C_ADDRESS);
        if (Wire.endTransmission() == 0) break;
        delay(5);
        yield();
    }

    bytesWritten += bytesToWrite;
    // Update global progress counter
    g_bytes_written += bytesToWrite;

    // small delay to allow EEPROM internal cycle
    delay(2);
    yield();
}

  // CRITICAL FIX: Do verification AFTER all bytes are written (if enabled)
  if (g_verify_after_write) {
    Serial.println("Starting verification...");
    bool verificationOk = true;
    
    for (int v = 0; v < length; v++) {
      uint16_t checkAddr = address + v;
      uint8_t expected = data[v];
      uint8_t actual = readFromEEPROM(checkAddr);
      
      if (expected != actual) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Verification mismatch at %u expected=0x%02X actual=0x%02X", checkAddr, expected, actual);
        Serial.println(buf);
        i2c_log_add(buf);
        
        // One retry
        delay(2);
        actual = readFromEEPROM(checkAddr);
        if (expected != actual) {
          char buf2[128];
          snprintf(buf2, sizeof(buf2), "Verification retry failed at %u expected=0x%02X actual=0x%02X", checkAddr, expected, actual);
          Serial.println(buf2);
          i2c_log_add(buf2);
          verificationOk = false;
          break;
        }
      }
      
      // Allow background tasks every 16 bytes
      if ((v & 0x0F) == 0) {
        yield();
      }
    }
    
    if (!verificationOk) {
      g_write_in_progress = false;
      setWriteProtect(true);
      return false;
    }
    Serial.println("Verification passed");
  }

  g_write_in_progress = false;
  setWriteProtect(true);
  Serial.print("writeToEEPROM(): success bytesWritten="); 
  Serial.println(bytesWritten);
  return true;
}

uint8_t readFromEEPROM(uint16_t address) {
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);
  Wire.write((int)(address >> 8));
  Wire.write((int)(address & 0xFF));
  Wire.endTransmission();

  Wire.requestFrom(EEPROM_I2C_ADDRESS, 1);
  if (Wire.available()) return Wire.read();
  return 0xFF;
}

void resetDSP() {
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);
  Wire.write(0xFF);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(100);
}

void setWriteProtect(bool enable) {
  if (EEPROM_WP_ACTIVE_HIGH) digitalWrite(EEPROM_WP_PIN, enable ? HIGH : LOW);
  else digitalWrite(EEPROM_WP_PIN, enable ? LOW : HIGH);
}

bool setDSPRunState(bool run) {
  uint8_t value = run ? (DSP_RUN_BIT_MASK) : (0x00);
  Serial.print("Attempting DSP I2C write to 0x"); Serial.print(DSP_I2C_ADDRESS, HEX);
  Serial.print(" reg=0x"); Serial.print(DSP_CONTROL_REGISTER, HEX);
  Serial.print(" val=0x"); Serial.println(value, HEX);

  Wire.beginTransmission(DSP_I2C_ADDRESS);
  Wire.write((uint8_t)DSP_CONTROL_REGISTER);
  Wire.write(value);
  int res = Wire.endTransmission();
  if (res == 0) return true;

  uint8_t alt = 0x1D;
  Serial.print("Primary DSP write failed (err="); Serial.print(res); Serial.println(") trying fallback 0x1D");
  Wire.beginTransmission(alt);
  Wire.write((uint8_t)DSP_CONTROL_REGISTER);
  Wire.write(value);
  int res2 = Wire.endTransmission();
  Serial.print("Fallback write result: "); Serial.println(res2);
  return (res2 == 0);
}

// CRITICAL FIX: Updated progress tracking functions
uint32_t getBytesWrittenProgress() { 
  return g_bytes_written; // Now returns the actual bytes written in current session
}

uint32_t getBytesToWrite() { 
  return g_total_bytes_to_write; 
}

bool isWriteInProgress() { 
  return g_write_in_progress; 
}

void setExpectedTotalBytes(uint32_t total) { 
  g_total_bytes_to_write = total; 
  // CRITICAL: Reset bytes written when starting a NEW upload
  g_bytes_written = 0;
}

void resetWriteProgress() {
  g_bytes_written = 0;
  g_total_bytes_to_write = 0;
  g_write_in_progress = false;
}

void setVerificationEnabled(bool enabled) {
  g_verify_after_write = enabled;
  char buf[64];
  snprintf(buf, sizeof(buf), "Verification set: %s", enabled ? "ON" : "OFF");
  Serial.println(buf);
  i2c_log_add(buf);
}

bool getVerificationEnabled() { 
  return g_verify_after_write; 
}