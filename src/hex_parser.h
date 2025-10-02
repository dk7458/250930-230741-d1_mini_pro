#ifndef HEX_PARSER_H
#define HEX_PARSER_H

#include <Arduino.h>

// Initialize hex parser state
void hex_begin();

// Process incoming hex data chunks
void processHexChunk(const char* chunk, size_t chunkLen);

// Force flush any remaining batched data to EEPROM
void flushBatch();

// Parse and write a single hex line
int parseAndWriteHexLine(const String &hexLine);

// Write complete hex data to EEPROM
int writeHexToEEPROM(const String &hexData);

int parseAndWriteCArrayLine(const String &line);

// Get statistics
int getTotalBytesWritten();
int getTotalLinesProcessed();

// Reset parser state
void resetUploadStats();

#endif