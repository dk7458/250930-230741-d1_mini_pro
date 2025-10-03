#include "web_routes.h"
#include "config.h"
#include "eeprom_manager.h"
#include "hex_parser.h"
#include "index.html.h"
#include <ArduinoJson.h>
#include <WebServer.h>
#include <Wire.h>

// Global server reference
static WebServer *g_server = nullptr;

// Upload state
static bool g_is_binary_upload = false;
static uint32_t g_binary_current_addr = 0;
static uint32_t g_expected_total_bytes = 0;

void sendJson(int code, const JsonDocument& doc) {
  String response;
  serializeJson(doc, response);
  g_server->send(code, "application/json", response);
}
bool checkMemorySafety() {
  if (ESP.getFreeHeap() < 2000) {
    Serial.printf("CRITICAL: Low memory - %d bytes free\n", ESP.getFreeHeap());
    return false;
  }
  return true;
}


// Helper functions for file type detection
bool isHexFile(const String& filename) {
  return filename.endsWith(".hex") || filename.endsWith(".txt");
}

bool isBinFile(const String& filename) {
  return filename.endsWith(".bin") || filename.endsWith(".rom");
}

// Simple route handlers
void handleRoot() {
  Serial.println("Serving main page");
  g_server->send(200, "text/html", MAIN_PAGE);
}

void handleDetect() {
  Serial.println("EEPROM detect");
  JsonDocument doc;
  doc["success"] = checkEEPROM();
  doc["message"] = doc["success"] ? "EEPROM detected" : "EEPROM not found";
  sendJson(200, doc);
}

void handleErase() {
  Serial.println("EEPROM erase");
  JsonDocument doc;
  doc["success"] = eraseEEPROM();
  doc["message"] = doc["success"] ? "EEPROM erased" : "Erase failed";
  sendJson(200, doc);
}

void handleWriteProtect() {
  Serial.println("Write protect");
  
  JsonDocument doc;
  if (!g_server->hasArg("plain")) {
    doc["success"] = false;
    doc["message"] = "No data";
    sendJson(400, doc);
    return;
  }
  
  String body = g_server->arg("plain");
  JsonDocument req;
  DeserializationError err = deserializeJson(req, body);
  
  if (err) {
    doc["success"] = false;
    doc["message"] = "Invalid JSON";
    sendJson(400, doc);
    return;
  }
  
  bool enable = req["enable"];
  setWriteProtect(enable);
  
  doc["success"] = true;
  doc["enabled"] = enable;
  sendJson(200, doc);
}

void handleDSPDiagnostic() {
  Serial.println("DSP comprehensive diagnostic");
  
  JsonDocument doc;
  doc["i2c_address"] = DSP_I2C_ADDRESS;
  doc["free_heap_start"] = ESP.getFreeHeap();
  
  // Test 1: Basic I2C communication
  Wire.beginTransmission(DSP_I2C_ADDRESS);
  int detectResult = Wire.endTransmission();
  doc["basic_detection"] = (detectResult == 0);
  doc["detect_result"] = detectResult;
  
  if (detectResult != 0) {
    doc["success"] = false;
    doc["message"] = "DSP not responding to basic I2C detection";
    sendJson(200, doc);
    return;
  }
  
  // Test 2: Read hardware ID with retry
  uint8_t hw_id = 0xFF;
  bool hw_id_valid = false;
  
  for (int attempt = 0; attempt < 3; attempt++) {
    Wire.beginTransmission(DSP_I2C_ADDRESS);
    Wire.write(0xF0); Wire.write(0x02); // Hardware ID register
    if (Wire.endTransmission(false) == 0) {
      if (Wire.requestFrom(DSP_I2C_ADDRESS, 1) == 1) {
        hw_id = Wire.read();
        hw_id_valid = (hw_id == 0x02);
        if (hw_id_valid) break;
      }
    }
    delay(5); // Short delay between attempts
    yield();
  }
  
  doc["hardware_id"] = hw_id;
  doc["hardware_valid"] = hw_id_valid;
  
  // Test 3: Try to write to control register
  bool write_success = false;
  Wire.beginTransmission(DSP_I2C_ADDRESS);
  Wire.write(0xF0); Wire.write(0x00); // Control register
  Wire.write(0x00); // Try to set running state
  int writeResult = Wire.endTransmission();
  doc["write_test"] = (writeResult == 0);
  doc["write_result"] = writeResult;
  
  // Test 4: Read back control register
  uint8_t control_read = 0xFF;
  if (writeResult == 0) {
    delay(10); // Allow write to complete
    Wire.beginTransmission(DSP_I2C_ADDRESS);
    Wire.write(0xF0); Wire.write(0x00);
    if (Wire.endTransmission(false) == 0) {
      if (Wire.requestFrom(DSP_I2C_ADDRESS, 1) == 1) {
        control_read = Wire.read();
      }
    }
  }
  doc["control_readback"] = control_read;
  
  doc["success"] = hw_id_valid;
  doc["message"] = hw_id_valid ? 
    "DSP communication established" : 
    "DSP detected but invalid hardware ID";
  doc["free_heap_end"] = ESP.getFreeHeap();
  
  sendJson(200, doc);
}

void handleDSPRun() {
  Serial.println("DSP run");
  
  JsonDocument doc;
  if (!g_server->hasArg("plain")) {
    doc["success"] = false;
    doc["message"] = "No data";
    sendJson(400, doc);
    return;
  }
  
  String body = g_server->arg("plain");
  JsonDocument req;
  DeserializationError err = deserializeJson(req, body);
  
  if (err) {
    doc["success"] = false;
    doc["message"] = "Invalid JSON";
    sendJson(400, doc);
    return;
  }
  
  bool run = req["run"];
  bool ok = setDSPRunState(run);
  
  doc["success"] = ok;
  doc["run"] = run;
  sendJson(200, doc);
}

void handleVerification() {
  if (g_server->method() == HTTP_GET) {
    JsonDocument doc;
    doc["enabled"] = getVerificationEnabled();
    sendJson(200, doc);
  } else if (g_server->method() == HTTP_POST) {
    JsonDocument doc;
    
    if (!g_server->hasArg("plain")) {
      doc["success"] = false;
      doc["message"] = "No data";
      sendJson(400, doc);
      return;
    }
    
    String body = g_server->arg("plain");
    JsonDocument req;
    DeserializationError err = deserializeJson(req, body);
    
    if (err) {
      doc["success"] = false;
      doc["message"] = "Invalid JSON";
    } else {
      bool enabled = req["enabled"];
      setVerificationEnabled(enabled);
      doc["success"] = true;
      doc["enabled"] = enabled;
    }
    sendJson(200, doc);
  }
}

void handleTestWrite() {
  Serial.println("Test write");
  
  JsonDocument doc;
  if (!g_server->hasArg("plain")) {
    doc["success"] = false;
    doc["message"] = "No data";
    sendJson(400, doc);
    return;
  }
  
  String body = g_server->arg("plain");
  JsonDocument req;
  DeserializationError err = deserializeJson(req, body);
  
  if (err) {
    doc["success"] = false;
    doc["message"] = "Invalid JSON";
    sendJson(400, doc);
    return;
  }
  
  uint32_t addr = req["address"];
  uint32_t val = req["value"];
  bool ok = testWriteByte(addr, val);
  
  doc["success"] = ok;
  doc["address"] = addr;
  doc["value"] = val;
  sendJson(200, doc);
}

void handleRead() {
  Serial.println("EEPROM read - optimized version");
  
  // Check memory first
  if (ESP.getFreeHeap() < 3000) {
    JsonDocument doc;
    doc["success"] = false;
    doc["message"] = "Insufficient memory for read operation";
    sendJson(500, doc);
    return;
  }
  
  // Stream response directly to avoid large string buffers
  g_server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  g_server->send(200, "application/json", "");
  
  // Start JSON response
  g_server->sendContent("{\"success\":true,\"data\":\"");
  
  // Read and stream in small chunks
  uint8_t readBuffer[16];  // Small buffer
  char hexBuffer[64];      // Buffer for hex conversion
  
  for (uint16_t baseAddr = 0; baseAddr < 256; baseAddr += 16) {
    // Read 16 bytes at once
    for (int i = 0; i < 16; i++) {
      readBuffer[i] = readFromEEPROM(baseAddr + i);
    }
    
    // Convert to hex string efficiently
    char *ptr = hexBuffer;
    for (int i = 0; i < 16; i++) {
      uint8_t b = readBuffer[i];
      *ptr++ = "0123456789ABCDEF"[b >> 4];
      *ptr++ = "0123456789ABCDEF"[b & 0x0F];
      *ptr++ = ' ';
    }
    
    // Replace last space with newline (except for last line)
    if (baseAddr < 240) {
      ptr[-1] = '\\';
      *ptr++ = 'n';
    } else {
      ptr--; // Remove trailing space
    }
    *ptr = '\0';
    
    // Send this line
    g_server->sendContent(hexBuffer);
    
    // Critical: yield frequently to prevent watchdog and memory issues
    yield();
    
    // Small delay to prevent I2C bus congestion
    delay(2);
    
    // Check memory every few lines
    if (baseAddr % 64 == 0 && ESP.getFreeHeap() < 2000) {
      g_server->sendContent("\",\"message\":\"Memory low - read incomplete\"}");
      Serial.println("Read aborted due to low memory");
      return;
    }
  }
  
  // End JSON response
  g_server->sendContent("\"}");
  
  Serial.println("EEPROM read completed successfully");
}

void handleDump() {
  Serial.println("EEPROM dump");
  g_server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  g_server->send(200, "application/octet-stream", "");
  
  uint8_t buf[64];
  for (uint16_t addr = 0; addr < EEPROM_SIZE; addr += 64) {
    for (uint8_t i = 0; i < 64; i++) {
      buf[i] = readFromEEPROM(addr + i);
    }
    g_server->sendContent((const char*)buf, 64);
    delay(1);
  }
}

void handleHeap() {
  JsonDocument doc;
  doc["free_heap"] = ESP.getFreeHeap();
  sendJson(200, doc);
}

void handleI2CScan() {
  JsonDocument doc;
  doc["found"] = i2c_scan();
  sendJson(200, doc);
}

void handleI2CLog() {
  JsonDocument doc;
  doc["log"] = i2c_log_get_all();
  sendJson(200, doc);
}

void handleProgress() {
  JsonDocument doc;
  doc["bytesWritten"] = getBytesWrittenProgress();
  doc["bytesTotal"] = getBytesToWrite();
  doc["inProgress"] = isWriteInProgress();
  sendJson(200, doc);
}

// Enhanced EEPROM operations
void handleReadRange() {
  Serial.println("EEPROM read range requested");
  
  JsonDocument doc;
  if (!g_server->hasArg("start") || !g_server->hasArg("length")) {
    doc["success"] = false;
    doc["message"] = "Missing start or length parameters";
    sendJson(400, doc);
    return;
  }
  
  uint16_t startAddr = g_server->arg("start").toInt();
  uint16_t length = g_server->arg("length").toInt();
  
  if (startAddr + length > EEPROM_SIZE) {
    doc["success"] = false;
    doc["message"] = "Read range exceeds EEPROM size";
    sendJson(400, doc);
    return;
  }
  
  if (length > 1024) {
    doc["success"] = false;
    doc["message"] = "Max read length is 1024 bytes";
    sendJson(400, doc);
    return;
  }
  
  String hexData = "";
  
  // Read data from EEPROM and build hex string
  for (uint16_t i = 0; i < length; i++) {
    uint8_t byteValue = readFromEEPROM(startAddr + i);
    
    // Convert to hex string
    if (byteValue < 16) hexData += "0";
    hexData += String(byteValue, HEX);
    
    if (i % 16 == 0) yield(); // Prevent watchdog timeout
  }
  
  doc["success"] = true;
  doc["startAddress"] = startAddr;
  doc["length"] = length;
  doc["hexData"] = hexData;
  
  sendJson(200, doc);
}
// ===== ENHANCED DSP CONTROL ROUTES =====

void handleDSPCoreRun() {
  Serial.println("DSP core run control - enhanced");
  
  JsonDocument doc;
  if (!g_server->hasArg("plain")) {
    doc["success"] = false;
    doc["message"] = "No data provided";
    sendJson(400, doc);
    return;
  }
  
  String body = g_server->arg("plain");
  JsonDocument req;
  DeserializationError err = deserializeJson(req, body);
  
  if (err) {
    doc["success"] = false;
    doc["message"] = "Invalid JSON";
    sendJson(400, doc);
    return;
  }
  
  bool run = req["run"];
  uint8_t value = run ? 0x00 : 0x01; // 0=running, 1=stopped/reset
  
  // Enhanced write with verification
  bool write_success = false;
  bool verify_success = false;
  
  for (int attempt = 0; attempt < 2; attempt++) {
    // Write to control register
    Wire.beginTransmission(DSP_I2C_ADDRESS);
    Wire.write(0xF0); Wire.write(0x00); // Control register 0xF000
    Wire.write(value);
    int result = Wire.endTransmission();
    
    if (result == 0) {
      write_success = true;
      delay(10); // Allow DSP to process
      
      // Verify write
      Wire.beginTransmission(DSP_I2C_ADDRESS);
      Wire.write(0xF0); Wire.write(0x00);
      if (Wire.endTransmission(false) == 0) {
        if (Wire.requestFrom(DSP_I2C_ADDRESS, (uint8_t)1) == 1) {
          uint8_t readback = Wire.read();
          verify_success = (readback == value);
          doc["readback_value"] = readback;
        }
      }
      
      if (verify_success) break;
    }
    
    delay(5); // Retry delay
    yield();
  }
  
  doc["success"] = write_success && verify_success;
  doc["run"] = run;
  doc["write_success"] = write_success;
  doc["verify_success"] = verify_success;
  doc["message"] = write_success ? 
    (verify_success ? 
      (run ? "DSP core started" : "DSP core stopped") : 
      "Write succeeded but verification failed") : 
    "DSP control write failed";
  
  sendJson(200, doc);
}

void handleDSPSoftReset() {
  Serial.println("DSP soft reset");
  
  JsonDocument doc;
  
  // Soft reset sequence: write 1 then 0 to core control register bit 0
  bool success = false;
  
  // First, stop the core
  Wire.beginTransmission(DSP_I2C_ADDRESS);
  Wire.write(0xF0); Wire.write(0x00); // Control register 0xF000
  Wire.write(0x01); // Set reset bit
  int result1 = Wire.endTransmission();
  
  if (result1 == 0) {
    delay(10); // Short delay for reset to take effect
    
    // Then clear reset bit to restart
    Wire.beginTransmission(DSP_I2C_ADDRESS);
    Wire.write(0xF0); Wire.write(0x00); // Control register 0xF000
    Wire.write(0x00); // Clear reset bit
    int result2 = Wire.endTransmission();
    
    success = (result2 == 0);
  }
  
  doc["success"] = success;
  doc["message"] = success ? "DSP soft reset completed" : "Soft reset failed";
  
  sendJson(200, doc);
}

void handleDSPSelfBoot() {
  Serial.println("DSP self-boot trigger");
  
  JsonDocument doc;
  
  // Self-boot sequence: ensure WP is high, then trigger reset
  setWriteProtect(true); // Ensure WP is high for self-boot
  
  // Use soft reset to trigger self-boot from EEPROM
  Wire.beginTransmission(DSP_I2C_ADDRESS);
  Wire.write(0xF0); Wire.write(0x00); // Control register 0xF000
  Wire.write(0x01); // Set reset bit
  int result1 = Wire.endTransmission();
  
  if (result1 == 0) {
    delay(50); // Allow time for boot process
    
    Wire.beginTransmission(DSP_I2C_ADDRESS);
    Wire.write(0xF0); Wire.write(0x00); // Control register 0xF000
    Wire.write(0x00); // Clear reset bit
    int result2 = Wire.endTransmission();
    
    doc["success"] = (result2 == 0);
    doc["message"] = (result2 == 0) ? "Self-boot initiated" : "Self-boot failed";
  } else {
    doc["success"] = false;
    doc["message"] = "Failed to trigger self-boot";
  }
  
  sendJson(200, doc);
}

void handleDSPI2CDebug() {
  Serial.println("DSP I2C debug toggle");
  
  JsonDocument doc;
  if (!g_server->hasArg("plain")) {
    doc["success"] = false;
    doc["message"] = "No data provided";
    sendJson(400, doc);
    return;
  }
  
  String body = g_server->arg("plain");
  JsonDocument req;
  DeserializationError err = deserializeJson(req, body);
  
  if (err) {
    doc["success"] = false;
    doc["message"] = "Invalid JSON";
    sendJson(400, doc);
    return;
  }
  
  bool enabled = req["enabled"];
  
  // This would control I2C debug logging - you'll need to implement
  // the actual debug mode in your I2C functions
  doc["success"] = true;
  doc["enabled"] = enabled;
  doc["message"] = enabled ? "I2C debug mode enabled" : "I2C debug mode disabled";
  
  sendJson(200, doc);
}

void handleDSPStatus() {
  Serial.println("DSP status read");
  
  JsonDocument doc;
  
  // Read core status register (0xF001)
  Wire.beginTransmission(DSP_I2C_ADDRESS);
  Wire.write(0xF0); Wire.write(0x01); // Status register 0xF001
  Wire.endTransmission(false);
  Wire.requestFrom(DSP_I2C_ADDRESS, 1);
  
  if (Wire.available()) {
    uint8_t status = Wire.read();
    doc["success"] = true;
    doc["dck_stable"] = (bool)(status & 0x01);     // Bit 0: DCK stable
    doc["sigmadsp_reset"] = (bool)(status & 0x02); // Bit 1: SigmaDSP reset
    doc["pll_locked"] = (bool)(status & 0x04);     // Bit 2: PLL locked
    doc["safeload_ready"] = (bool)(status & 0x08); // Bit 3: Safeload ready
    
    // Read core control register to get run state (0xF000)
    Wire.beginTransmission(DSP_I2C_ADDRESS);
    Wire.write(0xF0); Wire.write(0x00); // Control register 0xF000
    Wire.endTransmission(false);
    Wire.requestFrom(DSP_I2C_ADDRESS, 1);
    
    if (Wire.available()) {
      uint8_t control = Wire.read();
      doc["core_running"] = (bool)!(control & 0x01); // Inverted: 0=running, 1=reset
    }
    
    // Read hardware ID (0xF002) to verify communication
    Wire.beginTransmission(DSP_I2C_ADDRESS);
    Wire.write(0xF0); Wire.write(0x02); // Hardware ID register 0xF002
    Wire.endTransmission(false);
    Wire.requestFrom(DSP_I2C_ADDRESS, 1);
    
    if (Wire.available()) {
      uint8_t hw_id = Wire.read();
      doc["hardware_id"] = hw_id;
      doc["hardware_valid"] = (hw_id == 0x02); // ADAU1701 should return 0x02
    }
    
  } else {
    doc["success"] = false;
    doc["message"] = "DSP not responding";
  }
  
  sendJson(200, doc);
}

void handleDSPResetTest() {
  if (!checkMemorySafety()) {
    JsonDocument doc;
    doc["success"] = false;
    doc["message"] = "Insufficient memory";
    sendJson(500, doc);
    return;
  }
  
  JsonDocument doc;
  
  // Test: Try different I2C addresses in case of address conflict
  uint8_t possible_addresses[] = {0x34, 0x35, 0x36, 0x37};
  bool found = false;
  uint8_t working_addr = 0;
  
  for (int i = 0; i < 4; i++) {
    Wire.beginTransmission(possible_addresses[i]);
    int result = Wire.endTransmission();
    if (result == 0) {
      // Test hardware ID
      Wire.beginTransmission(possible_addresses[i]);
      Wire.write(0xF0); Wire.write(0x02);
      if (Wire.endTransmission(false) == 0) {
        // Use explicitly sized parameter to avoid ambiguity
        if (Wire.requestFrom(possible_addresses[i], (uint8_t)1) == 1) {
          uint8_t hw_id = Wire.read();
          if (hw_id == 0x02) {
            found = true;
            working_addr = possible_addresses[i];
            break;
          }
        }
      }
    }
    yield();
  }
  
  doc["success"] = found;
  doc["working_address"] = working_addr;
  doc["message"] = found ? 
    String("Found working address: 0x") + String(working_addr, HEX) : 
    "No valid DSP found at common addresses";
  
  sendJson(200, doc);
}

void handleDSPRegisters() {
  Serial.println("DSP register read - memory optimized");
  
  // Check memory before starting
  if (ESP.getFreeHeap() < 3000) {
    JsonDocument doc;
    doc["success"] = false;
    doc["message"] = "Insufficient memory for register read";
    sendJson(500, doc);
    return;
  }
  
  JsonDocument doc;
  bool anySuccess = false;
  
  // Read key DSP registers with better error handling
  const int NUM_REGS = 5;
  uint16_t regs[NUM_REGS] = {0xF000, 0xF001, 0xF002, 0xF003, 0xF004};
  
  for (int i = 0; i < NUM_REGS; i++) {
    // Use small local buffers to avoid memory fragmentation
    uint8_t regValue = 0xFF; // Default to 0xFF if read fails
    
    Wire.beginTransmission(DSP_I2C_ADDRESS);
    Wire.write((uint8_t)(regs[i] >> 8));   // High byte
    Wire.write((uint8_t)(regs[i] & 0xFF)); // Low byte
    int txResult = Wire.endTransmission(false);
    
    if (txResult == 0) {
      uint8_t bytesRead = Wire.requestFrom(DSP_I2C_ADDRESS, (uint8_t)1);
      if (bytesRead == 1 && Wire.available()) {
        regValue = Wire.read();
        anySuccess = true;
        
        // Log successful read for debugging
        Serial.printf("REG 0x%04X: 0x%02X\n", regs[i], regValue);
      }
    }
    
    // Add register to JSON document
    char regName[8];
    snprintf(regName, sizeof(regName), "0x%04X", regs[i]);
    doc["registers"][regName] = regValue;
    
    // Critical: yield and memory management
    yield();
    
    // Check memory every 2 registers
    if (i % 2 == 0 && ESP.getFreeHeap() < 2000) {
      doc["success"] = false;
      doc["message"] = "Memory exhausted during register read";
      sendJson(500, doc);
      return;
    }
    
    // Small delay for I2C bus stability
    delay(2);
  }
  
  doc["success"] = anySuccess;
  doc["message"] = anySuccess ? "Registers read successfully" : "Failed to read all registers";
  doc["heap_after"] = ESP.getFreeHeap(); // Debug info
  
  sendJson(200, doc);
}

void handleVerifyRange() {
  // Critical: Check memory before doing anything
  uint32_t startHeap = ESP.getFreeHeap();
  if (startHeap < 4000) {
    JsonDocument doc;
    doc["success"] = false;
    doc["message"] = "Insufficient memory: " + String(startHeap) + " bytes free";
    sendJson(500, doc);
    Serial.printf("VERIFY_REJECTED: Low memory - %d bytes free\n", startHeap);
    return;
  }
  
  Serial.println("EEPROM verify range - memory optimized");
  
  JsonDocument doc;
  if (!g_server->hasArg("plain")) {
    doc["success"] = false;
    doc["message"] = "No JSON data provided";
    sendJson(400, doc);
    return;
  }
  
  // Parse JSON with size limits
  String body = g_server->arg("plain");
  if (body.length() > 1024) { // Limit JSON size
    doc["success"] = false;
    doc["message"] = "Verification data too large";
    sendJson(400, doc);
    return;
  }
  
  JsonDocument req;
  DeserializationError err = deserializeJson(req, body);
  
  if (err) {
    doc["success"] = false;
    doc["message"] = "Invalid JSON format";
    sendJson(400, doc);
    return;
  }
  
  uint16_t startAddr = req["start"];
  uint16_t length = req["length"];
  String expectedHex = req["expectedData"] | "";
  
  // Validate parameters strictly
  if (startAddr >= EEPROM_SIZE) {
    doc["success"] = false;
    doc["message"] = "Start address exceeds EEPROM";
    sendJson(400, doc);
    return;
  }
  
  if (length == 0 || length > 256) { // Further reduce max size
    doc["success"] = false;
    doc["message"] = "Length must be 1-256 bytes";
    sendJson(400, doc);
    return;
  }
  
  if ((int)expectedHex.length() != length * 2) {
    doc["success"] = false;
    doc["message"] = "Hex data length mismatch";
    sendJson(400, doc);
    return;
  }
  
  // Verify with aggressive memory management
  bool verificationOk = true;
  int errors = 0;
  String errorDetails = "";
  
  for (uint16_t i = 0; i < length; i++) {
    // Extract expected byte efficiently
    char hexByte[3] = { expectedHex[i * 2], expectedHex[i * 2 + 1], '\0' };
    uint8_t expected = strtol(hexByte, NULL, 16);
    uint8_t actual = readFromEEPROM(startAddr + i);
    
    if (expected != actual) {
      if (errors < 3) { // Very limited error logging
        char errorMsg[48];
        snprintf(errorMsg, sizeof(errorMsg), "0x%04X: 0x%02X!=0x%02X", 
                 startAddr + i, expected, actual);
        errorDetails += String(errorMsg) + " ";
      }
      verificationOk = false;
      errors++;
    }
    
    // Aggressive yielding and memory checking
    if (i % 8 == 0) {
      yield();
      
      // Check memory every 8 bytes
      if (ESP.getFreeHeap() < 2500) {
        doc["success"] = false;
        doc["message"] = "Memory exhausted during verification";
        doc["bytesChecked"] = i + 1;
        doc["errorsFound"] = errors;
        sendJson(500, doc);
        Serial.printf("VERIFY_ABORTED: Memory at %d bytes, checked %d/%d\n", 
                     ESP.getFreeHeap(), i + 1, length);
        return;
      }
    }
  }
  
  doc["success"] = verificationOk;
  doc["bytesChecked"] = length;
  doc["errorsFound"] = errors;
  
  if (!verificationOk) {
    doc["message"] = String(errors) + " verification errors";
    if (errorDetails.length() > 0) {
      doc["errorDetails"] = errorDetails;
    }
  } else {
    doc["message"] = "Verification passed";
  }
  
  sendJson(200, doc);
  
  Serial.printf("VERIFY_COMPLETE: 0x%04X+%d, errors=%d, heap=%d->%d\n", 
                startAddr, length, errors, startHeap, ESP.getFreeHeap());
}

void handleStressTest() {
  Serial.println("EEPROM stress test requested");
  
  JsonDocument doc;
  int tests = g_server->hasArg("tests") ? g_server->arg("tests").toInt() : 10;
  
  if (tests > 100) tests = 100; // Limit for safety
  
  doc["testCount"] = tests;
  
  bool allPassed = true;
  int passedTests = 0;
  
  // Use a String to accumulate results, then assign once
  String results = "";
  
  for (int i = 0; i < tests; i++) {
    uint16_t addr = (i * 32) % (EEPROM_SIZE - 32); // Wrap around EEPROM
    uint8_t testValue = (i * 23) & 0xFF; // Simple pattern
    
    // Write test value
    uint8_t writeData[1] = {testValue};
    bool writeOk = writeToEEPROM(addr, writeData, 1);
    
    if (writeOk) {
      // Read back and verify
      uint8_t readValue = readFromEEPROM(addr);
      
      if (readValue == testValue) {
        passedTests++;
        results += String("Test ") + i + ": PASS (0x" + String(testValue, HEX) + " at 0x" + String(addr, HEX) + ")\n";
      } else {
        allPassed = false;
        results += String("Test ") + i + ": FAIL (wrote 0x" + String(testValue, HEX) + " got 0x" + String(readValue, HEX) + " at 0x" + String(addr, HEX) + ")\n";
      }
    } else {
      allPassed = false;
      results += String("Test ") + i + ": WRITE FAIL at 0x" + String(addr, HEX) + "\n";
    }
    
    yield(); // Allow other operations
  }
  
  doc["success"] = allPassed;
  doc["passedTests"] = passedTests;
  doc["totalTests"] = tests;
  doc["message"] = String("Stress test completed: ") + passedTests + "/" + tests + " passed";
  doc["results"] = results; // Assign the complete string at once
  
  sendJson(200, doc);
}

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
  
  sendJson(200, doc);
  
  // Reset upload state
  g_is_binary_upload = false;
  g_binary_current_addr = 0;
  g_expected_total_bytes = 0;
}

void register_web_routes(WebServer &server) {
  g_server = &server;

  // Main page
  server.on("/", HTTP_GET, handleRoot);

  // EEPROM operations
  server.on("/detect", HTTP_GET, handleDetect);
  server.on("/erase", HTTP_POST, handleErase);
  server.on("/read", HTTP_GET, handleRead);
  server.on("/dump", HTTP_GET, handleDump);
  
  // Enhanced EEPROM operations
  server.on("/read_range", HTTP_GET, handleReadRange);
  server.on("/verify_range", HTTP_POST, handleVerifyRange);
  server.on("/stress_test", HTTP_POST, handleStressTest);

  // Upload operations
  server.on("/upload_stream", HTTP_POST, handleUploadComplete, handleUploadStream);

  // Control operations
  server.on("/wp", HTTP_POST, handleWriteProtect);
  server.on("/dsp_run", HTTP_POST, handleDSPRun); // Keep legacy for compatibility
  server.on("/verification", HTTP_GET, handleVerification);
  server.on("/verification", HTTP_POST, handleVerification);
  server.on("/test_write", HTTP_POST, handleTestWrite);
  
  // ENHANCED DSP CONTROL ROUTES
  server.on("/dsp/core_run", HTTP_POST, handleDSPCoreRun);
  server.on("/dsp/soft_reset", HTTP_POST, handleDSPSoftReset);
  server.on("/dsp/self_boot", HTTP_POST, handleDSPSelfBoot);
  server.on("/dsp/i2c_debug", HTTP_POST, handleDSPI2CDebug);
  server.on("/dsp/status", HTTP_GET, handleDSPStatus);
  server.on("/dsp/diagnostic", HTTP_GET, handleDSPDiagnostic);
  server.on("/dsp/find", HTTP_GET, handleDSPResetTest);
  server.on("/dsp/find", HTTP_GET, handleDSPResetTest);
  server.on("/dsp/registers", HTTP_GET, handleDSPRegisters);

  // System operations
  server.on("/heap", HTTP_GET, handleHeap);
  server.on("/i2c_scan", HTTP_GET, handleI2CScan);
  server.on("/i2c_log", HTTP_GET, handleI2CLog);
  server.on("/progress", HTTP_GET, handleProgress);

  Serial.println("All routes registered - Enhanced DSP control available");
}