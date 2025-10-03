#include <Wire.h>
#include "eeprom_manager.h"
#include "dsp_helper.h"
#include "config.h"
#include <Arduino.h>

// I2C Activity Log (ring buffer)
static const int I2C_LOG_ENTRIES = 64;
static String i2c_log[I2C_LOG_ENTRIES];
static int i2c_log_idx = 0;

// Progress Tracking Variables
static volatile uint32_t g_bytes_written = 0;
static volatile uint32_t g_total_bytes_to_write = 0;
static volatile bool g_write_in_progress = false;
static volatile bool g_verify_after_write = false;

void i2c_log_add(const char* msg) {
    // Add timestamp for better tracking
    unsigned long timestamp = millis();
    char detailed_msg[128];
    snprintf(detailed_msg, sizeof(detailed_msg), "[%lu] %s", timestamp, msg);
    
    i2c_log[i2c_log_idx] = String(detailed_msg);
    i2c_log_idx = (i2c_log_idx + 1) % I2C_LOG_ENTRIES;
    
    // Also print to serial for debugging
    Serial.println(detailed_msg);
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
    yield(); // Keep ESP8266 happy
  }
  return out.length() > 0 ? out : String("none");
}

bool testWriteByte(uint16_t address, uint8_t value) {
  uint8_t pageBuf[1] = {value};
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

void eeprom_begin() {
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000); // Standard 100kHz
  pinMode(EEPROM_WP_PIN, OUTPUT);
  
  // Initialize WP pin to inactive state
  if (EEPROM_WP_ACTIVE_HIGH) {
    digitalWrite(EEPROM_WP_PIN, LOW);  // Write enabled
  } else {
    digitalWrite(EEPROM_WP_PIN, HIGH); // Write enabled
  }
  
  Serial.println("✓ I2C initialized");
}

bool checkEEPROM() {
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);
  int result = Wire.endTransmission();
  char msg[64];
  snprintf(msg, sizeof(msg), "EEPROM detect: addr=0x%02X, result=%d", EEPROM_I2C_ADDRESS, result);
  i2c_log_add(msg);
  return (result == 0);
}

bool eraseEEPROM() {
  uint8_t blankData[EEPROM_PAGE_SIZE];
  memset(blankData, 0xFF, EEPROM_PAGE_SIZE);
  
  setWriteProtect(false);
  bool ok = true;
  
  // Setup progress tracking
  g_bytes_written = 0;
  g_total_bytes_to_write = EEPROM_SIZE;
  g_write_in_progress = true;
  
  Serial.println("Starting EEPROM erase...");
  
  for (uint16_t addr = 0; addr < EEPROM_SIZE; addr += EEPROM_PAGE_SIZE) {
    if (!writeToEEPROM(addr, blankData, EEPROM_PAGE_SIZE)) {
      ok = false;
      break;
    }
    
    g_bytes_written += EEPROM_PAGE_SIZE;
    
    // Progress feedback every 4KB
    if ((addr % 4096) == 0) {
      Serial.printf("Erase progress: %d/%d bytes\n", g_bytes_written, EEPROM_SIZE);
    }
    
    yield();
    delay(2); // Small delay for stability
  }
  
  g_write_in_progress = false;
  setWriteProtect(true);
  
  if (ok) {
    Serial.println("✓ EEPROM erase completed");
    i2c_log_add("EEPROM erase: SUCCESS");
  } else {
    Serial.println("✗ EEPROM erase failed");
    i2c_log_add("EEPROM erase: FAILED");
  }
  
  return ok;
}

bool writeToEEPROM(uint16_t address, uint8_t data[], int length) {
  // Safety check: don't exceed EEPROM size
  if (address + length > EEPROM_SIZE) {
    char error_msg[80];
    snprintf(error_msg, sizeof(error_msg), "ERROR: Write exceeds EEPROM (0x%04X + %d > 0x%04X)", 
             address, length, EEPROM_SIZE);
    Serial.println(error_msg);
    i2c_log_add(error_msg);
    return false;
  }
  
  // Log the write operation start
  char log_msg[128];
  snprintf(log_msg, sizeof(log_msg), "I2C_WRITE_START: addr=0x%04X, len=%d, verify=%s", 
           address, length, g_verify_after_write ? "YES" : "NO");
  Serial.println(log_msg);
  i2c_log_add(log_msg);
  
  g_write_in_progress = true;
  setWriteProtect(false);

  int bytesWritten = 0;
  int totalBytes = length;
  unsigned long operationStart = millis();

  // CHUNK-BASED PROCESSING: Process in smaller chunks for better memory management
  const int WRITE_CHUNK_SIZE = 32;  // Optimal chunk size for memory/performance balance
  
  while (bytesWritten < totalBytes) {
    int remainingBytes = totalBytes - bytesWritten;
    int currentChunkSize = min(WRITE_CHUNK_SIZE, remainingBytes);
    uint16_t currentChunkAddr = address + bytesWritten;
    
    // Log chunk processing
    snprintf(log_msg, sizeof(log_msg), "I2C_CHUNK_WRITE: addr=0x%04X, chunk_size=%d, remaining=%d", 
             currentChunkAddr, currentChunkSize, remainingBytes);
    i2c_log_add(log_msg);

    // Write the current chunk using page-based algorithm
    int chunkBytesWritten = 0;
    while (chunkBytesWritten < currentChunkSize) {
      // Calculate how many bytes we can write in this page
      int pageOffset = (currentChunkAddr + chunkBytesWritten) % EEPROM_PAGE_SIZE;
      int pageRemaining = EEPROM_PAGE_SIZE - pageOffset;
      int bytesToWrite = min(pageRemaining, I2C_MAX_DATA_PER_XFER);
      bytesToWrite = min(bytesToWrite, currentChunkSize - chunkBytesWritten);

      // Log page write details (reduced verbosity for performance)
      #if EEPROM_DEBUG
      snprintf(log_msg, sizeof(log_msg), "I2C_PAGE_WRITE: page_addr=0x%04X, bytes=%d", 
               currentChunkAddr + chunkBytesWritten, bytesToWrite);
      i2c_log_add(log_msg);
      #endif

      // Start I2C transmission
      Wire.beginTransmission(EEPROM_I2C_ADDRESS);
      Wire.write((uint8_t)((currentChunkAddr + chunkBytesWritten) >> 8));   // High address byte
      Wire.write((uint8_t)((currentChunkAddr + chunkBytesWritten) & 0xFF)); // Low address byte
      
      #if EEPROM_DEBUG
      // Log the data being written (first few bytes for debugging) - only in debug mode
      if (bytesToWrite <= 4) {
        char data_hex[32] = "";
        for (int i = 0; i < bytesToWrite; i++) {
          char byte_hex[4];
          snprintf(byte_hex, sizeof(byte_hex), "%02X ", data[bytesWritten + chunkBytesWritten + i]);
          strcat(data_hex, byte_hex);
        }
        snprintf(log_msg, sizeof(log_msg), "I2C_WRITE_DATA: [%s]", data_hex);
        i2c_log_add(log_msg);
      }
      #endif
      
      Wire.write(data + bytesWritten + chunkBytesWritten, bytesToWrite);
      
      int result = Wire.endTransmission();
      
      #if EEPROM_DEBUG
      // Log I2C transmission result
      snprintf(log_msg, sizeof(log_msg), "I2C_TRANSMISSION: result=%d", result);
      i2c_log_add(log_msg);
      #endif
      
      if (result != 0) {
        snprintf(log_msg, sizeof(log_msg), "I2C_WRITE_ERROR: result=%d at address 0x%04X", 
                 result, currentChunkAddr + chunkBytesWritten);
        Serial.println(log_msg);
        i2c_log_add(log_msg);
        
        g_write_in_progress = false;
        setWriteProtect(true);
        return false;
      }

      // Wait for EEPROM to complete write cycle (poll for ACK)
      unsigned long writeStartTime = millis();
      bool writeCompleted = false;
      
      while (millis() - writeStartTime < 50) {
        Wire.beginTransmission(EEPROM_I2C_ADDRESS);
        int pollResult = Wire.endTransmission();
        
        if (pollResult == 0) {
          writeCompleted = true;
          break;
        }
        delay(5);
        yield();
      }
      
      if (!writeCompleted) {
        snprintf(log_msg, sizeof(log_msg), "I2C_WRITE_TIMEOUT: device not ready after 50ms at 0x%04X", 
                 currentChunkAddr + chunkBytesWritten);
        Serial.println(log_msg);
        i2c_log_add(log_msg);
        
        g_write_in_progress = false;
        setWriteProtect(true);
        return false;
      }

      chunkBytesWritten += bytesToWrite;
      yield(); // Allow background tasks
    }

    // VERIFY CURRENT CHUNK IMMEDIATELY (if verification enabled)
    if (g_verify_after_write) {
      bool chunkVerified = true;
      int verificationErrors = 0;
      
      // Verify this specific chunk immediately after writing
      for (int i = 0; i < currentChunkSize; i++) {
        uint8_t expected = data[bytesWritten + i];
        uint8_t actual = readFromEEPROM(currentChunkAddr + i);
        
        if (expected != actual) {
          if (verificationErrors < 3) { // Log first 3 errors only
            snprintf(log_msg, sizeof(log_msg), "I2C_VERIFY_ERROR: addr=0x%04X, exp=0x%02X, got=0x%02X", 
                     currentChunkAddr + i, expected, actual);
            Serial.println(log_msg);
            i2c_log_add(log_msg);
          }
          chunkVerified = false;
          verificationErrors++;
        }
        
        // Yield periodically to prevent watchdog
        if ((i % 8) == 0) yield();
      }
      
      if (!chunkVerified) {
        snprintf(log_msg, sizeof(log_msg), "I2C_VERIFY_FAILED: %d errors in chunk at 0x%04X", 
                 verificationErrors, currentChunkAddr);
        Serial.println(log_msg);
        i2c_log_add(log_msg);
        
        g_write_in_progress = false;
        setWriteProtect(true);
        return false;
      }
      
      #if EEPROM_DEBUG
      snprintf(log_msg, sizeof(log_msg), "I2C_CHUNK_VERIFIED: %d bytes at 0x%04X", 
               currentChunkSize, currentChunkAddr);
      Serial.println(log_msg);
      i2c_log_add(log_msg);
      #endif
      
      // MEMORY MANAGEMENT: This chunk is now verified - data can be discarded
      // The calling function should advance its buffer pointer to free memory
    }

    bytesWritten += currentChunkSize;
    
    // Update progress tracking
    g_bytes_written += currentChunkSize;
    
    // Log progress for large writes (less frequent to reduce overhead)
    if (totalBytes > 128 && (bytesWritten % 128 == 0)) {
      int percent = (bytesWritten * 100) / totalBytes;
      snprintf(log_msg, sizeof(log_msg), "I2C_WRITE_PROGRESS: %d/%d bytes (%d%%)", 
               bytesWritten, totalBytes, percent);
      Serial.println(log_msg);
      i2c_log_add(log_msg);
    }
    
    // Allow other operations between chunks
    yield();
    
    // Memory conservation: if heap is low, force garbage collection
    if (ESP.getFreeHeap() < 4000) {
      Serial.printf("LOW MEMORY: %d bytes free - pausing for cleanup\n", ESP.getFreeHeap());
      delay(10);
    }
  }

  g_write_in_progress = false;
  setWriteProtect(true);
  
  unsigned long totalTime = millis() - operationStart;
  snprintf(log_msg, sizeof(log_msg), "I2C_WRITE_FINISHED: %d bytes at 0x%04X, time=%lums, speed=%.1f B/s", 
           totalBytes, address, totalTime, 
           totalTime > 0 ? (totalBytes * 1000.0) / totalTime : 0);
  Serial.println(log_msg);
  i2c_log_add(log_msg);
  
  return true;
}

uint8_t readFromEEPROM(uint16_t address) {
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);
  Wire.write((uint8_t)(address >> 8));
  Wire.write((uint8_t)(address & 0xFF));
  Wire.endTransmission();
  
  Wire.requestFrom(EEPROM_I2C_ADDRESS, 1);
  if (Wire.available()) {
    return Wire.read();
  }
  return 0xFF; // Return 0xFF if read fails
}

void setWriteProtect(bool enable) {
  if (EEPROM_WP_ACTIVE_HIGH) {
    digitalWrite(EEPROM_WP_PIN, enable ? HIGH : LOW);
  } else {
    digitalWrite(EEPROM_WP_PIN, enable ? LOW : HIGH);
  }
  
  char msg[32];
  snprintf(msg, sizeof(msg), "Write Protect: %s", enable ? "ENABLED" : "DISABLED");
  i2c_log_add(msg);
}

// DSP Control Functions
// Moved to dsp_helper.cpp for better organization

// Progress Tracking Functions
uint32_t getBytesWrittenProgress() { 
  return g_bytes_written; 
}

uint32_t getBytesToWrite() { 
  return g_total_bytes_to_write; 
}

bool isWriteInProgress() { 
  return g_write_in_progress; 
}

void setExpectedTotalBytes(uint32_t total) { 
  g_total_bytes_to_write = total;
  g_bytes_written = 0; // Reset counter for new upload
}

void resetWriteProgress() {
  g_bytes_written = 0;
  g_total_bytes_to_write = 0;
  g_write_in_progress = false;
}

void setVerificationEnabled(bool enabled) {
  g_verify_after_write = enabled;
  Serial.printf("Verification: %s\n", enabled ? "ENABLED" : "DISABLED");
}

bool getVerificationEnabled() { 
  return g_verify_after_write; 
}

// Legacy DSP functions - moved to dsp_helper.cpp
// Kept for backward compatibility but now delegate to DSP helper