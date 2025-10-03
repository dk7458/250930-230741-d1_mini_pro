#include "upload_routes.h"
#include "config.h"
#include "eeprom_manager.h"
#include "hex_parser.h"
#include <ArduinoJson.h>
#include <WebServer.h>

// Global server reference (shared with other modules)
extern WebServer *g_server;

// Upload state
bool g_is_binary_upload = false;
uint32_t g_binary_current_addr = 0;
uint32_t g_expected_total_bytes = 0;

// Helper functions for file type detection (declared in web_routes.cpp)
extern bool isHexFile(const String& filename);
extern bool isBinFile(const String& filename);

// RELIABLE UPLOAD HANDLER - BIN focused
void handleUploadStream() {
  HTTPUpload& upload = g_server->upload();

  switch (upload.status) {
    case UPLOAD_FILE_START: {
      Serial.println("\n=== UPLOAD STARTED ===");
      Serial.printf("File: %s\n", upload.filename.c_str());

      // Detect file type
      g_is_binary_upload = isBinFile(upload.filename);
      g_binary_current_addr = 0;

      // Get expected size
      g_expected_total_bytes = 0;
      String qsize = g_server->arg("size");
      if (!qsize.isEmpty()) {
        g_expected_total_bytes = static_cast<uint32_t>(qsize.toInt());
      }

      Serial.printf("Type: %s, Expected: %u bytes\n",
                   g_is_binary_upload ? "BINARY" : "HEX",
                   g_expected_total_bytes);

      // Setup for upload
      resetUploadStats();
      resetWriteProgress();
      setWriteProtect(false);

      if (g_expected_total_bytes > 0) {
        setExpectedTotalBytes(g_expected_total_bytes);
      }
      break;
    }

    case UPLOAD_FILE_WRITE:
      if (upload.currentSize > 0) {
        if (g_is_binary_upload) {
          // BINARY MODE: Direct EEPROM write (memory efficient)
          if (g_binary_current_addr + upload.currentSize <= EEPROM_SIZE) {
            bool success = writeToEEPROM(g_binary_current_addr, upload.buf, upload.currentSize);
            if (success) {
              g_binary_current_addr += upload.currentSize;
              Serial.printf("BIN: wrote %d bytes at 0x%04X\n", upload.currentSize, g_binary_current_addr - upload.currentSize);
            } else {
              Serial.println("BIN: write failed!");
            }
          } else {
            Serial.println("BIN: would exceed EEPROM size!");
          }
        } else {
          // HEX MODE: Use parser (memory intensive)
          if (ESP.getFreeHeap() > 4000) { // Only process if we have enough memory
            processHexChunk(reinterpret_cast<const char*>(upload.buf), upload.currentSize);
          } else {
            Serial.println("HEX: skipping chunk - low memory");
          }
        }
      }
      break;

    case UPLOAD_FILE_END:
      Serial.println("=== UPLOAD COMPLETED ===");

      if (!g_is_binary_upload) {
        // Finalize HEX upload
        processHexChunk("", 0); // Flush buffer
        flushBatch();
      }

      setWriteProtect(true);
      Serial.printf("Final: %u bytes processed\n", getTotalBytesWritten());
      break;

    case UPLOAD_FILE_ABORTED:
      Serial.println("=== UPLOAD ABORTED ===");
      setWriteProtect(true);
      break;

    default:
      break;
  }
}

void handleUploadComplete() {
  JsonDocument doc;
  uint32_t bytesWritten = getTotalBytesWritten();

  bool success = (bytesWritten > 0);
  String message;

  if (success) {
    message = String("Upload successful: ") + bytesWritten + " bytes written";
  } else {
    message = "Upload failed - no data processed";

    // Special case for binary files
    if (g_is_binary_upload && g_binary_current_addr > 0) {
      message = String("Binary upload: ") + g_binary_current_addr + " bytes written";
      success = true;
      bytesWritten = g_binary_current_addr;
    }
  }

  doc["success"] = success;
  doc["bytesWritten"] = bytesWritten;
  doc["message"] = message;
  doc["fileType"] = g_is_binary_upload ? "binary" : "hex";

  // Reset upload state
  g_is_binary_upload = false;
  g_binary_current_addr = 0;
  g_expected_total_bytes = 0;

  // Send response using external helper
  extern void sendJson(int code, const JsonDocument& doc);
  sendJson(200, doc);
}

void register_upload_routes(WebServer &server) {
  // Upload operations
  server.on("/upload_stream", HTTP_POST, handleUploadComplete, handleUploadStream);
}