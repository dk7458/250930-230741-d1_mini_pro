# Web Routes Modular Architecture

This project has been refactored to use a modular architecture for better maintainability and organization.

## Directory Structure

```
src/
├── html/                    # HTML templates and static content
│   └── index.html.h        # Main web interface
├── eeprom_routes.h/.cpp    # EEPROM operations (detect, read, write, verify)
├── dsp_routes.h/.cpp       # DSP control operations (run, reset, status)
├── upload_routes.h/.cpp    # File upload handling (HEX/BIN files)
├── system_routes.h/.cpp    # System status operations (heap, I2C scan)
├── web_routes.h/.cpp       # Main routing coordinator and shared utilities
└── [other core files...]
```

## Module Responsibilities

### EEPROM Routes (`eeprom_routes.*`)
- `/detect` - EEPROM presence detection
- `/erase` - EEPROM chip erase
- `/read` - Full EEPROM dump
- `/dump` - Binary EEPROM dump
- `/read_range` - Read specific address range
- `/verify_range` - Verify EEPROM contents against expected data
- `/stress_test` - EEPROM reliability testing

### DSP Routes (`dsp_routes.*`)
- `/dsp_run` - Legacy DSP run control
- `/dsp/core_run` - Enhanced DSP core control with verification
- `/dsp/soft_reset` - DSP soft reset
- `/dsp/self_boot` - Trigger DSP self-boot from EEPROM
- `/dsp/i2c_debug` - I2C debug mode toggle
- `/dsp/status` - DSP status register read
- `/dsp/diagnostic` - Comprehensive DSP diagnostics
- `/dsp/find` - DSP address discovery
- `/dsp/registers` - DSP register dump

### Upload Routes (`upload_routes.*`)
- `/upload_stream` - File upload handler (HEX/BIN files)
- Upload state management
- File type detection and processing

### System Routes (`system_routes.*`)
- `/heap` - Free heap memory status
- `/i2c_scan` - I2C bus device scan
- `/i2c_log` - I2C communication logs
- `/progress` - Upload/write progress tracking

### Main Web Routes (`web_routes.*`)
- Route registration coordination
- Shared utilities (`sendJson`, `checkMemorySafety`)
- Main page serving (`/`)
- Control operations (`/wp`, `/verification`, `/test_write`)

## Benefits of Modular Architecture

1. **Maintainability**: Each module focuses on a specific domain
2. **Testability**: Individual modules can be tested in isolation
3. **Reusability**: Modules can be reused in other projects
4. **Collaboration**: Multiple developers can work on different modules
5. **Code Organization**: Related functionality is grouped together

## Adding New Routes

1. Create new module files (`new_module.h/.cpp`)
2. Implement route handlers in the new module
3. Add registration function (`register_new_module_routes`)
4. Include the new header in `web_routes.h`
5. Call the registration function in `register_web_routes`

## Backward Compatibility

The refactored code maintains full backward compatibility with existing APIs and external interfaces.