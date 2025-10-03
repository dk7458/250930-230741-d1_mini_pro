#include "dsp_routes.h"
#include "config.h"
#include <ArduinoJson.h>
#include <WebServer.h>
#include <Wire.h>
#include "eeprom_manager.h"

// Global server reference (shared with other modules)
extern WebServer *g_server;

// Helper functions (shared with other modules)
extern void sendJson(int code, const JsonDocument& doc);
extern bool checkMemorySafety();

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
        if (Wire.requestFrom((uint8_t)DSP_I2C_ADDRESS, (uint8_t)1) == 1) {
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
  Wire.requestFrom((uint8_t)DSP_I2C_ADDRESS, (uint8_t)1);

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
    Wire.requestFrom((uint8_t)DSP_I2C_ADDRESS, (uint8_t)1);

    if (Wire.available()) {
      uint8_t control = Wire.read();
      doc["core_running"] = (bool)!(control & 0x01); // Inverted: 0=running, 1=reset
    }

    // Read hardware ID (0xF002) to verify communication
    Wire.beginTransmission(DSP_I2C_ADDRESS);
    Wire.write(0xF0); Wire.write(0x02); // Hardware ID register 0xF002
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)DSP_I2C_ADDRESS, (uint8_t)1);

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
      if (Wire.requestFrom((uint8_t)DSP_I2C_ADDRESS, (uint8_t)1) == 1) {
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
      if (Wire.requestFrom((uint8_t)DSP_I2C_ADDRESS, (uint8_t)1) == 1) {
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
        if (Wire.requestFrom((uint8_t)possible_addresses[i], (uint8_t)1) == 1) {
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
      uint8_t bytesRead = Wire.requestFrom((uint8_t)DSP_I2C_ADDRESS, (uint8_t)1);
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

void register_dsp_routes(WebServer &server) {
  // DSP control operations
  server.on("/dsp_run", HTTP_POST, handleDSPRun); // Keep legacy for compatibility

  // ENHANCED DSP CONTROL ROUTES
  server.on("/dsp/core_run", HTTP_POST, handleDSPCoreRun);
  server.on("/dsp/soft_reset", HTTP_POST, handleDSPSoftReset);
  server.on("/dsp/self_boot", HTTP_POST, handleDSPSelfBoot);
  server.on("/dsp/i2c_debug", HTTP_POST, handleDSPI2CDebug);
  server.on("/dsp/status", HTTP_GET, handleDSPStatus);
  server.on("/dsp/diagnostic", HTTP_GET, handleDSPDiagnostic);
  server.on("/dsp/find", HTTP_GET, handleDSPResetTest);
  server.on("/dsp/registers", HTTP_GET, handleDSPRegisters);
}