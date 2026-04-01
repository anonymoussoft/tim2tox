// Callback bridge for Dart ReceivePort communication
#pragma once

#if defined(_WIN32) || defined(_WIN64)
  // MSVC: the Dart SDK headers may not be present in some CI environments.
  // Keep this header compilable by falling back to minimal stubs.
#endif

#if defined(__has_include)
#  if __has_include(<dart_api_dl.h>)
#    include <dart_api_dl.h>
#    define TIM2TOX_HAS_DART_API_DL 1
#  else
#    define TIM2TOX_HAS_DART_API_DL 0
#  endif
#else
#  define TIM2TOX_HAS_DART_API_DL 0
#endif

#if !TIM2TOX_HAS_DART_API_DL
// Minimal Dart API DL stubs for builds where <dart_api_dl.h> isn't available.
// These are sufficient for compilation; runtime Dart messaging won't work.
#include <cstdint>
using Dart_Port = int64_t;
static constexpr Dart_Port ILLEGAL_PORT = -1;

enum Dart_CObject_Type { Dart_CObject_kString = 0 };

struct Dart_CObject {
    Dart_CObject_Type type;
    union {
        const char* as_string;
    } value;
};

static inline bool Dart_InitializeApiDL(void* /*data*/) { return false; }
static inline bool Dart_PostCObject_DL(Dart_Port /*port*/, Dart_CObject* /*obj*/) { return false; }
#endif

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

