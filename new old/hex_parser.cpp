#include "hex_parser.h"
#include "config.h"
#include "eeprom_manager.h"
#include <Arduino.h>

// D1 Mini Optimized - Batch processing for massive speed improvement
static int totalBytesWritten = 0;
static int totalLinesProcessed = 0;
static String buffer = "";
static uint32_t currentWriteAddress = 0;

// Batch processing - write multiple hex lines in one I2C transaction
const size_t BATCH_BUFFER_SIZE = 64;  // Match EEPROM page size for maximum efficiency
static uint8_t batchBuffer[BATCH_BUFFER_SIZE];
static uint16_t batchStartAddr = 0xFFFF; // Invalid start address
static size_t batchBytes = 0;

// Memory management optimized for ESP8266
const size_t MAX_BUFFER_SIZE = 512;    // Reduced further for D1 Mini
const size_t MIN_FREE_HEAP = 4000;     // Conservative for ESP8266

void hex_begin() {
  totalBytesWritten = 0;
  totalLinesProcessed = 0;
  buffer = "";
  buffer.reserve(256);  // Smaller pre-allocation for ESP8266
  currentWriteAddress = 0;
  batchStartAddr = 0xFFFF;
  batchBytes = 0;
}

// Flush batch buffer to EEPROM
void flushBatch() {
  if (batchBytes > 0 && batchStartAddr != 0xFFFF) {
    // CRITICAL DEBUG: Show what we're writing
    Serial.printf("BATCH WRITE: addr=0x%04X, bytes=%d [", batchStartAddr, batchBytes);
    for (size_t i = 0; i < batchBytes && i < 8; i++) {
      Serial.printf("%02X ", batchBuffer[i]);
    }
    if (batchBytes > 8) Serial.print("...");
    Serial.println("]");
    
    bool success = writeToEEPROM(batchStartAddr, batchBuffer, batchBytes);
    if (success) {
      totalBytesWritten += batchBytes;
      Serial.printf("Batch write SUCCESS: %d bytes at 0x%04X\n", batchBytes, batchStartAddr);
    } else {
      Serial.printf("Batch write FAILED: %d bytes at 0x%04X\n", batchBytes, batchStartAddr);
    }
    batchBytes = 0;
    batchStartAddr = 0xFFFF;
    
    // Critical for ESP8266: yield after each write
    yield();
  }
}

// Add data to batch buffer, flush if needed
void addToBatch(uint16_t address, const uint8_t* data, size_t length) {
  for (size_t i = 0; i < length; i++) {
    uint16_t currentAddr = address + i;
    
    // If this address doesn't continue the current batch, flush it
    if (batchStartAddr != 0xFFFF && 
        (currentAddr != batchStartAddr + batchBytes || batchBytes >= BATCH_BUFFER_SIZE)) {
      flushBatch();
    }
    
    // Start new batch if needed
    if (batchStartAddr == 0xFFFF) {
      batchStartAddr = currentAddr;
      batchBytes = 0;
    }
    
    // Add byte to batch
    batchBuffer[batchBytes] = data[i];
    batchBytes++;
  }
}

void processHexChunk(const char* chunk, size_t chunkLen) {
  // ESP8266 Memory check
  if (ESP.getFreeHeap() < MIN_FREE_HEAP) {
    Serial.printf("LOW MEM: %d bytes free, skipping chunk\n", ESP.getFreeHeap());
    delay(5);
    return;
  }

  // Efficient string conversion
  String chunkStr;
  chunkStr.reserve(chunkLen + 1);
  for (size_t i = 0; i < chunkLen; i++) {
    if (chunk[i] != '\0') chunkStr += chunk[i];
  }

  chunkStr.replace("\r\n", "\n");
  chunkStr.replace("\r", "\n");

  // Prevent buffer overflow
  if (buffer.length() + chunkStr.length() > MAX_BUFFER_SIZE) {
    Serial.printf("BUFFER FULL: %d+%d, processing...\n", buffer.length(), chunkStr.length());
    processExistingBuffer();
    buffer = "";
    buffer.reserve(256);
  }

  buffer += chunkStr;
  processExistingBuffer();
}

void processExistingBuffer() {
  int newlinePos;
  int linesProcessed = 0;
  
  while ((newlinePos = buffer.indexOf('\n')) >= 0) {
    String line = buffer.substring(0, newlinePos);
    line.trim();
    buffer = buffer.substring(newlinePos + 1);
    linesProcessed++;

    if (line.length() > 0) {
      if (line.startsWith(":")) {
        parseAndWriteHexLine(line);
      } else if (line.indexOf("0x") >= 0) {
        parseAndWriteRawLine(line);
      }
      // Silently skip unrecognized lines for speed
    }

    // ESP8266: Yield every few lines to prevent WDT
    if (linesProcessed % 2 == 0) {
      yield();
    }
    
    // Emergency memory check
    if (ESP.getFreeHeap() < 3000) {
      Serial.println("CRITICAL MEMORY - breaking processing");
      break;
    }
  }
}

int parseAndWriteHexLine(const String &hexLine) {
  // Quick length check
  if (hexLine.length() < 11) return 0;

  // Parse hex line efficiently
  int byteCount = strtol(hexLine.substring(1, 3).c_str(), NULL, 16);
  uint16_t address = strtol(hexLine.substring(3, 7).c_str(), NULL, 16);
  int recordType = strtol(hexLine.substring(7, 9).c_str(), NULL, 16);

  // CRITICAL DEBUG: Show address progression
  Serial.printf("HEX: addr=0x%04X (%5d), bytes=%2d, type=0x%02X", 
                address, address, byteCount, recordType);
  
  // Handle record types
  if (recordType == 0x01) { // EOF
    Serial.println(" [EOF]");
    flushBatch(); // Ensure all data is written before EOF
    return 0;
  }
  
  if (recordType == 0x00 && byteCount > 0) {
    // Validate line length
    if (hexLine.length() < 11 + byteCount * 2) {
      Serial.println(" [INCOMPLETE]");
      return 0;
    }
    
    Serial.println(" [DATA]");
    
    // Parse data directly into batch - no temporary allocation
    for (int i = 0; i < byteCount; i++) {
      String byteStr = hexLine.substring(9 + i * 2, 11 + i * 2);
      uint8_t byteVal = strtol(byteStr.c_str(), NULL, 16);
      addToBatch(address + i, &byteVal, 1);
    }
    
    totalLinesProcessed++;
    return byteCount;
  }
  
  Serial.println(" [OTHER]");
  return 0;
}

int parseAndWriteRawLine(const String &rawLine) {
  int bytesWritten = 0;
  String processedLine = rawLine;
  processedLine.replace(" ", "");
  
  int i = 0;
  while (i < processedLine.length()) {
    int idx = processedLine.indexOf("0x", i);
    if (idx == -1) break;
    if (idx + 4 <= processedLine.length()) {
      String byteStr = processedLine.substring(idx + 2, idx + 4);
      uint8_t byteVal = strtol(byteStr.c_str(), NULL, 16);
      addToBatch(currentWriteAddress, &byteVal, 1);
      bytesWritten++;
      currentWriteAddress++;
      i = idx + 4;
    } else {
      break;
    }
    
    // Yield periodically
    if (bytesWritten % 8 == 0) yield();
  }
  
  return bytesWritten;
}

int writeHexToEEPROM(const String &hexData) {
  hex_begin(); // Reset state
  processHexChunk(hexData.c_str(), hexData.length());
  flushBatch(); // Ensure all data is written
  return totalBytesWritten;
}

int getTotalBytesWritten() { 
  return totalBytesWritten; 
}

int getTotalLinesProcessed() { 
  return totalLinesProcessed; 
}

void resetUploadStats() { 
  hex_begin(); // Reuse initialization
}