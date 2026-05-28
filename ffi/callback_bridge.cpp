#include "callback_bridge.h"
#include "V2TIMLog.h"
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <csignal>
#if !defined(_WIN32) && !defined(__ANDROID__) && !defined(TIM2TOX_DISABLE_BACKTRACE)
#include <execinfo.h>
#include <unistd.h>
#endif
#include <cstdlib>

// Store Dart_Port for sending callbacks
static Dart_Port g_dart_port = ILLEGAL_PORT;
static std::mutex g_dart_port_mutex;
static bool g_dart_api_initialized = false;

static void PrintBacktraceOnSignal(int sig) {
#if defined(_WIN32) || defined(__ANDROID__) || defined(TIM2TOX_DISABLE_BACKTRACE)
    fprintf(stderr, "\n[callback_bridge] FATAL: received signal %d\n", sig);
    fflush(stderr);
    std::exit(128 + sig);
#else
    void* frames[64];
    int n = backtrace(frames, 64);
    fprintf(stderr, "\n[callback_bridge] FATAL: received signal %d\n", sig);
    backtrace_symbols_fd(frames, n, STDERR_FILENO);
    fprintf(stderr, "[callback_bridge] FATAL: end backtrace\n");
    fflush(stderr);
    _exit(128 + sig);
#endif
}

static void InstallCrashHandlersOnce() {
    static bool installed = false;
    if (installed) return;
    installed = true;
    signal(SIGSEGV, PrintBacktraceOnSignal);
    signal(SIGABRT, PrintBacktraceOnSignal);
#ifdef SIGBUS
    signal(SIGBUS, PrintBacktraceOnSignal);
#endif
}

extern "C" {
    // Initialize Dart API
    // Function signature must match native_imsdk_bindings_generated.dart:
    // int DartInitDartApiDL(Pointer<Void> data)
    int DartInitDartApiDL(void* data) {
        if (!data) {
            return 1;
        }

        InstallCrashHandlersOnce();

        // Dart_InitializeApiDL returns intptr_t: 0 = success, non-zero = failure
        // (missing symbols or ABI mismatch). Without a successful init,
        // Dart_PostCObject_DL must not be called, so propagate the failure
        // instead of silently flipping g_dart_api_initialized.
        intptr_t init_rc = Dart_InitializeApiDL(data);
        if (init_rc != 0) {
            return 1;
        }
        g_dart_api_initialized = true;
        return 0;
    }
    
    // Register Dart SendPort for receiving callbacks
    // Function signature must match native_imsdk_bindings_generated.dart:
    // void DartRegisterSendPort(int64_t send_port)
    // Note: Dart_Port is int64_t (64-bit), not int (32-bit)
    void DartRegisterSendPort(int64_t send_port) {
        std::lock_guard<std::mutex> lock(g_dart_port_mutex);
        g_dart_port = static_cast<Dart_Port>(send_port);
    }

    // Symmetric counterpart to DartRegisterSendPort. The Tencent SDK started
    // calling this in 8.9.7540+3 from NativeLibraryManager.unregisterPort().
    // No toxee code path invokes it today, but the symbol must exist so the
    // FFI lookup doesn't crash if it ever fires.
    void DartUnregisterSendPort(int64_t send_port) {
        std::lock_guard<std::mutex> lock(g_dart_port_mutex);
        if (g_dart_port == static_cast<Dart_Port>(send_port)) {
            g_dart_port = ILLEGAL_PORT;
        }
    }
}

// Check if Dart port is registered
bool IsDartPortRegistered() {
    std::lock_guard<std::mutex> lock(g_dart_port_mutex);
    return g_dart_port != ILLEGAL_PORT;
}

// Send callback message to Dart layer
// The message format must match what NativeLibraryManager._handleNativeMessage expects:
// - JSON string with "callback" field ("globalCallback" or "apiCallback")
// - For globalCallback: contains "callbackType" and other JSON data fields
// - For apiCallback: contains "user_data" and result data
void SendCallbackToDart(const char* callback_type, const std::string& json_data, void* user_data) {
    if (!callback_type) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_dart_port_mutex);
    if (!g_dart_api_initialized || g_dart_port == ILLEGAL_PORT || !callback_type) {
        return;
    }

    std::string message = json_data;
    Dart_CObject cobj;
    cobj.type = Dart_CObject_kString;

    size_t message_len = message.length();

    if (message_len > 1024 * 1024) {
        V2TIM_LOG(kError, "[callback_bridge] SendCallbackToDart: ERROR - message_len too large: {}", message_len);
        return;
    }

    char* message_cstr = static_cast<char*>(malloc(message_len + 1));
    if (!message_cstr) {
        V2TIM_LOG(kError, "[callback_bridge] SendCallbackToDart: ERROR - malloc failed for message_len={}", message_len);
        return;
    }

    const char* message_cstr_src = message.c_str();
    if (!message_cstr_src) {
        message_cstr[0] = '\0';
    } else {
        std::memcpy(message_cstr, message_cstr_src, message_len);
        message_cstr[message_len] = '\0';
    }

    cobj.value.as_string = message_cstr;
    // Dart_PostCObject_DL copies the CObject graph (including the kString
    // payload) into the receiving isolate's heap before returning, so the
    // caller retains ownership of message_cstr regardless of post success.
    Dart_PostCObject_DL(g_dart_port, &cobj);
    free(message_cstr);
}
