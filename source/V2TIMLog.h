#ifndef V2TIM_LOG_H
#define V2TIM_LOG_H

#include <string>
#include <fstream>
#include <mutex>
#include <sstream>
#include <memory>
#include <cstdio> // For potential future use or compatibility if needed

// Define log levels
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

// Use specific log level constants for clarity in calls
constexpr LogLevel kDebug = LogLevel::DEBUG;
constexpr LogLevel kInfo = LogLevel::INFO;
constexpr LogLevel kWarning = LogLevel::WARNING;
constexpr LogLevel kError = LogLevel::ERROR;
constexpr LogLevel kFatal = LogLevel::FATAL;


// Singleton Logger Class
class V2TIMLog {
public:
    static V2TIMLog& getInstance();

    // Configuration methods
    void setLogFile(const std::string& path);
    void setLogLevel(LogLevel level);
    void enableConsoleOutput(bool enable);

    // Logging methods using variadic templates
    template<typename... Args>
    void Debug(const char* format, Args... args) {
        log(LogLevel::DEBUG, format, args...);
    }

    template<typename... Args>
    void Info(const char* format, Args... args) {
        log(LogLevel::INFO, format, args...);
    }

    template<typename... Args>
    void Warning(const char* format, Args... args) {
        log(LogLevel::WARNING, format, args...);
    }

    template<typename... Args>
    void Error(const char* format, Args... args) {
        log(LogLevel::ERROR, format, args...);
    }

    template<typename... Args>
    void Fatal(const char* format, Args... args) {
        log(LogLevel::FATAL, format, args...);
    }

    // Make log method public so it can be called via the macro
    template<typename... Args>
    void log(LogLevel level, const char* format, Args... args) {
        if (level < min_level_) return;

        std::stringstream ss;
        ss << "[" << getLevelString(level) << "] ";
        // Use the recursive template function for formatting
        formatMessage(ss, format, args...);
        writeLog(ss.str());
    }

private:
    // Private constructor/destructor for singleton
    V2TIMLog();
    ~V2TIMLog();

    // Disable copy/move semantics
    V2TIMLog(const V2TIMLog&) = delete;
    V2TIMLog& operator=(const V2TIMLog&) = delete;
    V2TIMLog(V2TIMLog&&) = delete;
    V2TIMLog& operator=(V2TIMLog&&) = delete;

    // Helper methods
    void writeLog(const std::string& message);
    const char* getLevelString(LogLevel level);

    // Recursive template function for formatting messages
    void formatMessage(std::stringstream& ss, const char* format); // Base case

    template<typename T, typename... Args>
    void formatMessage(std::stringstream& ss, const char* format, T value, Args... args) {
        const char* p = format;
        while (*p) {
            if (*p == '{' && *(p + 1) == '}') {
                ss << value;
                formatMessage(ss, p + 2, args...); // Recurse with the rest
                return;
            }
            ss << *p++;
        }
        // If format string ends but there are still arguments, append them directly (optional behavior)
         ss << value; // Append the current value
         formatMessage(ss, "", args...); // Continue with remaining args and empty format
    }


    // Member variables
    std::unique_ptr<std::ofstream> log_file_;
    LogLevel min_level_;
    bool console_output_;
    std::mutex mutex_;
};

// Macros for easy logging
// Usage: V2TIMLog(kInfo, "User {} logged in from IP {}", userID, ipAddress);
// #define V2TIMLog(level, ...) V2TIMLog::getInstance().log(level, __VA_ARGS__)
// 暂时注释掉宏定义，以后再统一解决冲突问题

// 添加宏定义，用于简化日志调用
#define V2TIM_LOG(level, ...) \
    do { \
        if (level == kDebug) V2TIMLog::getInstance().Debug(__VA_ARGS__); \
        else if (level == kInfo) V2TIMLog::getInstance().Info(__VA_ARGS__); \
        else if (level == kWarning) V2TIMLog::getInstance().Warning(__VA_ARGS__); \
        else if (level == kError) V2TIMLog::getInstance().Error(__VA_ARGS__); \
        else if (level == kFatal) V2TIMLog::getInstance().Fatal(__VA_ARGS__); \
    } while(0)


#endif // V2TIM_LOG_H 