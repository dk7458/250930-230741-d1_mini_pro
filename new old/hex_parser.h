#ifndef HEX_PARSER_H
#define HEX_PARSER_H

#include <Arduino.h>

void hex_begin();
void processHexChunk(const char* chunk, size_t chunkLen);
void processExistingBuffer(); // Added declaration
int parseAndWriteHexLine(const String &hexLine);
int parseAndWriteRawLine(const String &rawLine);
int writeHexToEEPROM(const String &hexData);

int getTotalBytesWritten();
int getTotalLinesProcessed();
void resetUploadStats();

#endif