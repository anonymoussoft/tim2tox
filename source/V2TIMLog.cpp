#include "V2TIMLog.h"
#include <iostream>
#include <ctime>
#include <iomanip>

// Define the static instance getter
V2TIMLog& V2TIMLog::getInstance() {
    static V2TIMLog instance; // Thread-safe in C++11 and later
    return instance;
}

// Constructor
V2TIMLog::V2TIMLog()
    : min_level_(LogLevel::INFO), // Default log level
      console_output_(true)       // Default to console output enabled
{
    // Optional: Log initialization message
    // Info("V2TIMLog initialized.");
}

// Destructor: Close the log file if open
V2TIMLog::~V2TIMLog() {
    if (log_file_ && log_file_->is_open()) {
        log_file_->close();
    }
}

// Set the path for the log file
void V2TIMLog::setLogFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
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
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = level;
}

// Enable or disable logging to the console
void V2TIMLog::enableConsoleOutput(bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    console_output_ = enable;
}

// Core function to write the formatted log message
void V2TIMLog::writeLog(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Get current time
    auto now = std::time(nullptr);
    #ifdef _WIN32
        // Windows specific localtime_s
        std::tm tm_buf;
        localtime_s(&tm_buf, &now);
        auto* tm = &tm_buf;
    #else
        // POSIX compliant localtime_r or fallback to localtime
        #ifdef __unix__
            std::tm tm_buf;
            auto* tm = localtime_r(&now, &tm_buf);
        #else
            // Fallback if localtime_r is not available (less thread-safe)
            auto* tm = std::localtime(&now);
        #endif
    #endif

    std::stringstream timestamp;
    if (tm) { // Check if localtime conversion was successful
         timestamp << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    } else {
        timestamp << "[Timestamp Error]";
    }

    // Add timestamp and newline
    std::string full_message = timestamp.str() + " " + message + "\n";

    // Write to file if configured and open
    if (log_file_ && log_file_->is_open()) {
        *log_file_ << full_message;
        log_file_->flush(); // Ensure immediate write
    }

    // Output to console if enabled
    if (console_output_) {
        #ifdef _WIN32
            // Consider OutputDebugStringA on Windows for debugger output
            // OutputDebugStringA(full_message.c_str());
             std::cout << full_message; // Also print to standard console
        #else
             std::cout << full_message;
        #endif
    }
}

// Get the string representation of a log level
const char* V2TIMLog::getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
        default:               return "UNKNOWN";
    }
}

// Base case for recursive formatMessage template
void V2TIMLog::formatMessage(std::stringstream& ss, const char* format) {
    ss << format; // Append the remaining format string
} 