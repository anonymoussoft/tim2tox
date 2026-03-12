#include "V2TIMLog.h"
#include <iostream>
#include <ctime>
#include <iomanip>
#include <atomic>
#include <thread>
#include <chrono>
#include <sstream>
#include <functional>
#include <unordered_map>
#if defined(_WIN32) || defined(_WIN64)
#include <process.h>
#include <windows.h>
#define getpid _getpid
#else
#include <unistd.h>
#if defined(__linux__)
#include <sys/syscall.h>
#endif
#endif

// On macOS/iOS only: map each std::thread::id to a short, process-unique integer (1, 2, 3, …).
#if defined(__APPLE__)
static std::unordered_map<std::thread::id, int> s_tid_map;
static int s_tid_next = 1;
#endif

// Define the static instance getter
V2TIMLog& V2TIMLog::getInstance() {
    static V2TIMLog instance; // Thread-safe in C++11 and later
    return instance;
}

// Constructor
V2TIMLog::V2TIMLog()
    : min_level_(LogLevel::INFO),
      console_output_(true),
      destroyed_(false),
      sequence_(0)
{
}

// Destructor: Close the log file if open
V2TIMLog::~V2TIMLog() {
    // Mark as destroyed to prevent any further logging attempts
    destroyed_.store(true);
    // Wait a bit to ensure any in-flight log operations complete
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (log_file_ && log_file_->is_open()) {
        log_file_->close();
    }
}

// Set the path for the log file
void V2TIMLog::setLogFile(const std::string& path) {
    if (destroyed_.load()) return;
    std::lock_guard<std::mutex> lock(mutex_);
    if (destroyed_.load()) return;
    // Close existing file if open
    if (log_file_ && log_file_->is_open()) {
        log_file_->close();
    }
    // Attempt to open the new file
    log_file_ = std::make_unique<std::ofstream>(path, std::ios::app);
    if (!log_file_ || !log_file_->is_open()) {
        // Log error to console if file opening fails
        enableConsoleOutput(true); // Ensure console output is on for the error
        Error("Failed to open log file: {}", path);
        log_file_.reset(); // Reset pointer if opening failed
    }
}

// Set the minimum log level to record
void V2TIMLog::setLogLevel(LogLevel level) {
    if (destroyed_.load()) return;
    std::lock_guard<std::mutex> lock(mutex_);
    if (destroyed_.load()) return;
    min_level_ = level;
}

// Enable or disable logging to the console
void V2TIMLog::enableConsoleOutput(bool enable) {
    if (destroyed_.load()) return;
    std::lock_guard<std::mutex> lock(mutex_);
    if (destroyed_.load()) return;
    console_output_ = enable;
}

// Core function: unified format [pid-tid-seqno] YYYY-MM-DD HH:MM:SS [LEVEL] body
void V2TIMLog::writeLog(LogLevel level, const std::string& body) {
    if (destroyed_.load()) return;
    std::lock_guard<std::mutex> lock(mutex_);
    if (destroyed_.load()) return;

    const uint64_t seq = sequence_++;
    const int pid = static_cast<int>(getpid());

    std::string tid_str;
#if defined(__APPLE__)
    // macOS/iOS: use short process-unique integer instead of long hex thread id
    auto real_tid = std::this_thread::get_id();
    auto it = s_tid_map.find(real_tid);
    if (it == s_tid_map.end()) {
        it = s_tid_map.emplace(real_tid, s_tid_next++).first;
    }
    tid_str = std::to_string(it->second);
#elif defined(_WIN32) || defined(_WIN64)
    // Windows: kernel thread id (integer)
    tid_str = std::to_string(static_cast<unsigned long long>(GetCurrentThreadId()));
#elif defined(__linux__)
    // Linux: kernel thread id (integer), gettid(2) via syscall
    tid_str = std::to_string(static_cast<long>(syscall(SYS_gettid)));
#else
    // Other (e.g. BSD): fallback to process-unique integer from thread::id hash
    tid_str = std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
#endif

    auto now = std::time(nullptr);
#ifdef _WIN32
    std::tm tm_buf;
    localtime_s(&tm_buf, &now);
    auto* tm = &tm_buf;
#else
#ifdef __unix__
    std::tm tm_buf;
    auto* tm = localtime_r(&now, &tm_buf);
#else
    auto* tm = std::localtime(&now);
#endif
#endif

    std::stringstream timestamp;
    if (tm) {
        timestamp << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    } else {
        timestamp << "[Timestamp Error]";
    }

    // [pid-tid-seqno] YYYY-MM-DD HH:MM:SS [LEVEL] body
    std::stringstream line;
    line << "[" << pid << "-" << tid_str << "-" << seq << "] "
         << timestamp.str() << " [" << getLevelString(level) << "] " << body << "\n";
    std::string full_message = line.str();

    if (log_file_ && log_file_->is_open()) {
        *log_file_ << full_message;
        log_file_->flush();
    }

    if (console_output_) {
        std::cout << full_message;
    }
}

// Get the string representation of a log level (unified: DEBUG/INFO/WARN/ERROR/FATAL)
const char* V2TIMLog::getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

// Base case for recursive formatMessage template
void V2TIMLog::formatMessage(std::stringstream& ss, const char* format) {
    ss << format; // Append the remaining format string
} 