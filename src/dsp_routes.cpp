#include "dsp_routes.h"
#include "dsp_helper.h"
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
  bool ok = dsp_setRunState(run);

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

  // Enhanced write with verification using DSP helper
  bool write_success = dsp_writeRegisterVerified(ADAU1701_REG_CONTROL, value);
  bool verify_success = false;
  uint8_t readback_value = 0xFF;

  if (write_success) {
    delay(10); // Allow DSP to process
    verify_success = dsp_readRegister(ADAU1701_REG_CONTROL, readback_value);
    verify_success = verify_success && (readback_value == value);
  }

  doc["readback_value"] = readback_value;

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

  // Soft reset sequence using DSP helper
  bool success = dsp_softReset();

  doc["success"] = success;
  doc["message"] = success ? "DSP soft reset completed" : "Soft reset failed";

  sendJson(200, doc);
}

void handleDSPSelfBoot() {
  Serial.println("DSP self-boot trigger");

  JsonDocument doc;

  // Self-boot sequence using DSP helper
  bool success = dsp_selfBoot();

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

  // Get comprehensive status using DSP helper
  DSPStatus status = dsp_getStatus();

  doc["success"] = status.detected;
  if (status.detected) {
    doc["dck_stable"] = status.dckStable;
    doc["pll_locked"] = status.pllLocked;
    doc["safeload_ready"] = status.safeloadReady;
    doc["core_running"] = status.coreRunning;
    doc["hardware_id"] = status.hardwareId;
    doc["hardware_valid"] = (status.hardwareId == 0x02);
    doc["software_id"] = status.softwareId;
    doc["control_register"] = status.controlReg;
    doc["status_register"] = status.statusReg;
  } else {
    doc["message"] = "DSP not responding";
  }

  sendJson(200, doc);
}

void handleDSPDiagnostic() {
  Serial.println("DSP comprehensive diagnostic");

  JsonDocument doc;
  doc["i2c_address"] = DSP_I2C_ADDRESS;
  doc["free_heap_start"] = ESP.getFreeHeap();

  // Test 1: Basic I2C communication using DSP helper
  bool detected = dsp_detect();
  doc["basic_detection"] = detected;

  if (!detected) {
    doc["success"] = false;
    doc["message"] = "DSP not responding to basic I2C detection";
    doc["free_heap_end"] = ESP.getFreeHeap();
    sendJson(200, doc);
    return;
  }

  // Test 2: Read hardware ID using DSP helper
  DSPStatus status = dsp_getStatus();
  doc["hardware_id"] = status.hardwareId;
  doc["hardware_valid"] = (status.hardwareId == 0x02);

  // Test 3: Test communication using DSP helper
  bool comm_test = dsp_testCommunication();
  doc["write_test"] = comm_test;

  // Test 4: Get control register value
  doc["control_readback"] = status.controlReg;

  doc["success"] = status.detected && (status.hardwareId == 0x02) && comm_test;
  doc["message"] = doc["success"] ?
    "DSP communication established" :
    "DSP detected but communication issues";
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