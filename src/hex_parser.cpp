#include "hex_parser.h"
#include "config.h"
#include "eeprom_manager.h"
#include <Arduino.h>

// Parser state variables
static int totalBytesWritten = 0;
static int totalLinesProcessed = 0;
static String buffer = "";
static uint32_t currentWriteAddress = 0;

// Batch processing for efficiency
const size_t BATCH_BUFFER_SIZE = 64;  // Match EEPROM page size
static uint8_t batchBuffer[BATCH_BUFFER_SIZE];
static uint16_t batchStartAddr = 0xFFFF; // Invalid start address
static size_t batchBytes = 0;

// Memory management for ESP8266
const size_t MAX_BUFFER_SIZE = 512;
const size_t MIN_FREE_HEAP = 4000;

void hex_begin() {
  totalBytesWritten = 0;
  totalLinesProcessed = 0;
  buffer = "";
  buffer.reserve(256);
  currentWriteAddress = 0;
  batchStartAddr = 0xFFFF;
  batchBytes = 0;
  
  Serial.println("✓ Hex parser initialized");
}

void flushBatch() {
  if (batchBytes > 0 && batchStartAddr != 0xFFFF) {
    Serial.printf("Flushing batch: 0x%04X, %d bytes\n", batchStartAddr, batchBytes);
    
    bool success = writeToEEPROM(batchStartAddr, batchBuffer, batchBytes);
    if (success) {
      totalBytesWritten += batchBytes;
      Serial.printf("✓ Batch write: %d bytes at 0x%04X\n", batchBytes, batchStartAddr);
    } else {
      Serial.printf("✗ Batch write failed: %d bytes at 0x%04X\n", batchBytes, batchStartAddr);
    }
    
    batchBytes = 0;
    batchStartAddr = 0xFFFF;
    yield(); // Allow background tasks
  }
}

void addToBatch(uint16_t address, const uint8_t* data, size_t length) {
  for (size_t i = 0; i < length; i++) {
    uint16_t currentAddr = address + i;
    
    // If this address doesn't continue current batch, flush it
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

// Replace processHexChunk with memory-optimized version:
void processHexChunk(const char* chunk, size_t chunkLen) {
  // More aggressive memory management for ESP8266
  if (ESP.getFreeHeap() < 5000) { // Increased threshold
    Serial.printf("LOW MEMORY: %d bytes free - skipping chunk\n", ESP.getFreeHeap());
    delay(10);
    return;
  }

  // Process character by character to avoid large string allocations
  for (size_t i = 0; i < chunkLen; i++) {
    char c = chunk[i];
    if (c == '\r') continue; // Skip carriage returns
    
    if (c == '\n' || buffer.length() >= 128) { // Smaller buffer size
      if (buffer.length() > 0) {
        buffer.trim();
        
        if (buffer.length() > 0) {
          if (buffer.startsWith(":")) {
            parseAndWriteHexLine(buffer);
          } else if (buffer.indexOf("0x") >= 0) {
            // C array format - add this function
            parseAndWriteCArrayLine(buffer);
          }
          // Free memory immediately
          buffer = "";
          buffer.reserve(128);
        }
      }
      if (c == '\n') continue;
    }
    
    buffer += c;
  }
  
  // Force garbage collection if memory is critical
  if (ESP.getFreeHeap() < 3000) {
    Serial.println("CRITICAL MEMORY - forcing cleanup");
    buffer = "";
    flushBatch();
    delay(10);
  }
}

// Add C array parser function:
int parseAndWriteCArrayLine(const String &line) {
  String cleanLine = line;
  cleanLine.replace(" ", "");
  cleanLine.replace(",", " ");
  cleanLine.trim();
  
  int bytesWritten = 0;
  int startIndex = 0;
  
  while (startIndex < (int)cleanLine.length()) {
    int hexIndex = cleanLine.indexOf("0x", startIndex);
    if (hexIndex == -1) break;
    
if (hexIndex + 4 <= (int)cleanLine.length()) {
      String byteStr = cleanLine.substring(hexIndex + 2, hexIndex + 4);
      uint8_t byteVal = strtol(byteStr.c_str(), NULL, 16);
      addToBatch(currentWriteAddress, &byteVal, 1);
      bytesWritten++;
      currentWriteAddress++;
      startIndex = hexIndex + 4;
    } else {
      break;
    }
    
    // Prevent memory issues with large lines
    if (bytesWritten % 16 == 0) {
      yield();
    }
  }
  
  if (bytesWritten > 0) {
    Serial.printf("C Array: %d bytes at 0x%04X\n", bytesWritten, currentWriteAddress - bytesWritten);
  }
  
  return bytesWritten;
}

int parseAndWriteHexLine(const String &hexLine) {
  // Basic validation
  if (hexLine.length() < 11) {
    return 0;
  }

  // Parse HEX record fields
  int byteCount = strtol(hexLine.substring(1, 3).c_str(), NULL, 16);
  uint16_t address = strtol(hexLine.substring(3, 7).c_str(), NULL, 16);
  int recordType = strtol(hexLine.substring(7, 9).c_str(), NULL, 16);

  Serial.printf("HEX: 0x%04X (%d), bytes=%d, type=0x%02X", address, address, byteCount, recordType);
  
  // Handle different record types
  if (recordType == 0x01) { // End of File
    Serial.println(" [EOF]");
    flushBatch(); // Ensure all data is written
    return 0;
  }
  
  if (recordType == 0x00 && byteCount > 0) { // Data record
    // Validate we have enough data
    if ((int)hexLine.length() < 11 + byteCount * 2) {
      Serial.println(" [INCOMPLETE]");
      return 0;
    }
    
    Serial.println(" [DATA]");
    
    // Parse and batch the data bytes
    for (int i = 0; i < byteCount; i++) {
      String byteStr = hexLine.substring(9 + i * 2, 11 + i * 2);
      uint8_t byteVal = strtol(byteStr.c_str(), NULL, 16);
      addToBatch(address + i, &byteVal, 1);
    }
    
    totalLinesProcessed++;
    return byteCount;
  }
  
  // Handle other record types (extended address, etc.)
  if (recordType == 0x04) { // Extended Linear Address
    uint16_t upperAddress = strtol(hexLine.substring(9, 13).c_str(), NULL, 16);
    currentWriteAddress = (uint32_t)upperAddress << 16;
    Serial.printf(" [EXT_ADDR: 0x%04X]\n", upperAddress);
  } else if (recordType == 0x02) { // Extended Segment Address
    uint16_t segmentAddress = strtol(hexLine.substring(9, 13).c_str(), NULL, 16);
    currentWriteAddress = (uint32_t)segmentAddress << 4;
    Serial.printf(" [SEG_ADDR: 0x%04X]\n", segmentAddress);
  } else {
    Serial.printf(" [UNKNOWN: 0x%02X]\n", recordType);
  }
  
  return 0;
}

// Legacy function - kept for compatibility
int parseAndWriteRawLine(const String &rawLine) {
  return 0; // Not implemented in this version
}

int writeHexToEEPROM(const String &hexData) {
  hex_begin(); // Reset state
  processHexChunk(hexData.c_str(), hexData.length());
  flushBatch(); // Final flush
  return totalBytesWritten;
}

int getTotalBytesWritten() { 
  return totalBytesWritten; 
}

int getTotalLinesProcessed() { 
  return totalLinesProcessed; 
}

void resetUploadStats() { 
  hex_begin();
}