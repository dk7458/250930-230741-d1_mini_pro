#ifndef HEX_PARSER_H
#define HEX_PARSER_H

#include "eeprom_manager.h"

class HexParser {
public:
    static int writeHexToEEPROM(String hexData) {
        int bytesWritten = 0;
        int lineCount = 0;
        int totalLines = 0;
        
        for (int i = 0; i < hexData.length(); i++) {
            if (hexData[i] == '\n') totalLines++;
        }
        
        Serial.println("Processing HEX file with approximately " + String(totalLines) + " lines");
        
        while (hexData.length() > 0) {
            int newlinePos = hexData.indexOf('\n');
            if (newlinePos == -1) newlinePos = hexData.length();
            
            String line = hexData.substring(0, newlinePos);
            line.trim();
            
            if (line.length() > 0 && line.startsWith(":")) {
                bytesWritten += parseAndWriteHexLine(line);
                lineCount++;
            }
            
            hexData = hexData.substring(newlinePos + 1);
            
            if (lineCount % 50 == 0) {
                Serial.println("Processed " + String(lineCount) + " lines, written " + String(bytesWritten) + " bytes");
                delay(1);
            }
        }
        
        Serial.println("✅ HEX processing complete: " + String(bytesWritten) + " bytes written from " + String(lineCount) + " lines");
        return bytesWritten;
    }

private:
    static int parseAndWriteHexLine(String hexLine) {
        if (hexLine.length() < 11) return 0;
        
        String byteCountStr = hexLine.substring(1, 3);
        int byteCount = strtol(byteCountStr.c_str(), NULL, 16);
        
        String addressStr = hexLine.substring(3, 7);
        uint16_t address = strtol(addressStr.c_str(), NULL, 16);
        
        String recordTypeStr = hexLine.substring(7, 9);
        int recordType = strtol(recordTypeStr.c_str(), NULL, 16);
        
        if (recordType == 0x00) {
            uint8_t data[byteCount];
            for (int i = 0; i < byteCount; i++) {
                String byteStr = hexLine.substring(9 + i * 2, 11 + i * 2);
                data[i] = strtol(byteStr.c_str(), NULL, 16);
            }
            
            if (EEPROMManager::writeToEEPROM(address, data, byteCount)) {
                return byteCount;
            } else {
                Serial.println("❌ Write failed for address: 0x" + String(address, HEX));
            }
        } else if (recordType == 0x01) {
            Serial.println("📄 End of HEX file record found");
        }
        
        return 0;
    }
};

#endif
