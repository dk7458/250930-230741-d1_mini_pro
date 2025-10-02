#ifndef EEPROM_MANAGER_H
#define EEPROM_MANAGER_H

#include "config.h"

class EEPROMManager {
public:
    static bool checkEEPROM() {
        Wire.beginTransmission(EEPROM_I2C_ADDRESS);
        byte error = Wire.endTransmission();
        return (error == 0);
    }
    
    static bool eraseEEPROM() {
        uint8_t blankData[EEPROM_PAGE_SIZE];
        memset(blankData, 0xFF, EEPROM_PAGE_SIZE);
        
        for (uint16_t addr = 0; addr < EEPROM_SIZE; addr += EEPROM_PAGE_SIZE) {
            uint16_t chunkSize = min(EEPROM_PAGE_SIZE, EEPROM_SIZE - addr);
            if (!writeToEEPROM(addr, blankData, chunkSize)) {
                Serial.println("❌ Erase failed at address: 0x" + String(addr, HEX));
                return false;
            }
            delay(10);
            
            if (addr % 1024 == 0) {
                Serial.println("Erase progress: " + String(addr) + "/" + String(EEPROM_SIZE) + " bytes");
            }
        }
        return true;
    }
    
    static bool writeToEEPROM(uint16_t address, uint8_t data[], int length) {
        int bytesWritten = 0;
        
        while (bytesWritten < length) {
            Wire.beginTransmission(EEPROM_I2C_ADDRESS);
            Wire.write((int)((address + bytesWritten) >> 8));
            Wire.write((int)((address + bytesWritten) & 0xFF));
            
            int bytesToWrite = min(EEPROM_PAGE_SIZE - ((address + bytesWritten) % EEPROM_PAGE_SIZE), 
                                  length - bytesWritten);
            
            for (int i = 0; i < bytesToWrite; i++) {
                Wire.write(data[bytesWritten + i]);
            }
            
            byte error = Wire.endTransmission();
            if (error != 0) {
                Serial.println("❌ I2C write error: " + String(error));
                return false;
            }
            
            bytesWritten += bytesToWrite;
            delay(10);
        }
        return true;
    }
    
    static uint8_t readFromEEPROM(uint16_t address) {
        Wire.beginTransmission(EEPROM_I2C_ADDRESS);
        Wire.write((int)(address >> 8));
        Wire.write((int)(address & 0xFF));
        byte error = Wire.endTransmission();
        
        if (error != 0) {
            Serial.println("❌ I2C read setup error: " + String(error));
            return 0xFF;
        }
        
        Wire.requestFrom(EEPROM_I2C_ADDRESS, 1);
        if (Wire.available()) {
            return Wire.read();
        }
        return 0xFF;
    }
    
    static bool advancedEEPROMCheck() {
        uint16_t testAddr = 0x100;
        uint8_t testData = 0xA5;
        uint8_t originalData = readFromEEPROM(testAddr);
        
        uint8_t writeData[] = {testData};
        if (!writeToEEPROM(testAddr, writeData, 1)) {
            return false;
        }
        delay(10);
        
        uint8_t readBack = readFromEEPROM(testAddr);
        
        writeData[0] = originalData;
        writeToEEPROM(testAddr, writeData, 1);
        
        return (readBack == testData);
    }
    
    static String readEEPROMRange(uint16_t startAddr, uint16_t length) {
        String data = "";
        for (uint16_t addr = startAddr; addr < startAddr + length; addr++) {
            uint8_t byteData = readFromEEPROM(addr);
            if (byteData < 16) data += "0";
            data += String(byteData, HEX) + " ";
            if ((addr - startAddr + 1) % 16 == 0) data += "\n";
        }
        return data;
    }
};

#endif
