#ifndef DSP_ROUTES_H
#define DSP_ROUTES_H

#include <WebServer.h>

// DSP route handlers
void handleDSPRun();
void handleDSPCoreRun();
void handleDSPSoftReset();
void handleDSPSelfBoot();
void handleDSPI2CDebug();
void handleDSPStatus();
void handleDSPDiagnostic();
void handleDSPResetTest();
void handleDSPRegisters();

// DSP routes registration
void register_dsp_routes(WebServer &server);

#endif // DSP_ROUTES_H