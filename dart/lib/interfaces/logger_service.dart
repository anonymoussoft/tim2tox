/// Abstract interface for logging
/// 
/// This allows the framework to work with different logging implementations
/// without being tied to a specific package.
abstract class LoggerService {
  /// Log a message at info level
  void log(String message);
  
  /// Log an error with stack trace
  void logError(String message, Object error, StackTrace stack);
  
  /// Log a warning message
  void logWarning(String message);
  
  /// Log a debug message
  void logDebug(String message);
}

