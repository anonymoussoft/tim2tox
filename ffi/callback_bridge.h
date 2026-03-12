// Callback bridge for Dart ReceivePort communication
#pragma once

#include <dart_api_dl.h>
#include <string>
#include <mutex>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize Dart API
// Returns 0 on success, non-zero on failure
int DartInitDartApiDL(void* data);

// Register Dart SendPort for receiving callbacks
// Note: Dart_Port is int64_t (64-bit), not int (32-bit)
void DartRegisterSendPort(int64_t send_port);

#ifdef __cplusplus
}
#endif

// Send callback message to Dart layer
// callback_type: "globalCallback" or "apiCallback"
// json_data: JSON string containing callback data
// user_data: user data pointer (can be nullptr)
void SendCallbackToDart(const char* callback_type, const std::string& json_data, void* user_data);

// Check if Dart port is registered
bool IsDartPortRegistered();

