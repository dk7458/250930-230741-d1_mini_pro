#ifndef UPLOAD_ROUTES_H
#define UPLOAD_ROUTES_H

#include <WebServer.h>

// Upload state (shared across upload handlers)
extern bool g_is_binary_upload;
extern uint32_t g_binary_current_addr;
extern uint32_t g_expected_total_bytes;

// Upload route handlers
void handleUploadStream();
void handleUploadComplete();

// Upload routes registration
void register_upload_routes(WebServer &server);

#endif // UPLOAD_ROUTES_H