#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <string>
#include <vector>
#include <chrono>
#include <mutex>
#include <fstream>
#include <memory>
#include <functional>
#include <queue>
#include <atomic>
#include <thread>
#include <condition_variable>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

enum class ErrorSeverity {
    INFO = 0,
    WARNING = 1,
    ERROR = 2,
    CRITICAL = 3,
    FATAL = 4
};

enum class ErrorCategory {
    GENERAL,
    SEARCH_ENGINE,
    UI_SYSTEM,
    DATA_STORAGE,
    NETWORK,
    MEMORY,
    FILE_IO,
    TRANSLATION,
    PRESENTATION,
    CRASH_RECOVERY,
    BACKUP_SYSTEM,
    HEALTH_MONITORING
};

struct ErrorEvent {
    std::string id;
    ErrorSeverity severity;
    ErrorCategory category;
    std::string message;
    std::string context;
    std::string stack_trace;
    std::chrono::system_clock::time_point timestamp;
    std::string session_id;
    json additional_data;
    bool user_reported = false;
    bool auto_reported = false;
    int occurrence_count = 1;
};

struct ErrorPattern {
    std::string pattern_id;
    std::string message_pattern;
    ErrorCategory category;
    std::string suggested_solution;
    std::string user_friendly_message;
    bool auto_recoverable = false;
    std::function<bool()> recovery_action;
    int max_occurrences = 10;
    std::chrono::minutes cooldown_period{5};
};

struct ErrorStats {
    int total_errors = 0;
    int total_warnings = 0;
    int total_critical = 0;
    int total_fatal = 0;
    int resolved_errors = 0;
    int auto_recovered = 0;
    std::chrono::seconds average_resolution_time{0};
    double error_rate = 0.0; // errors per hour
};

class ErrorHandler {
private:
    std::string log_file_path;
    std::string error_report_dir;
    std::atomic<bool> is_initialized{false};
    std::atomic<bool> is_running{false};
    
    // Logging
    std::ofstream log_file;
    std::mutex log_mutex;
    
    // Error tracking
    std::vector<ErrorEvent> error_history;
    std::mutex error_mutex;
    int max_history_size = 1000;
    
    // Error patterns and recovery
    std::vector<ErrorPattern> error_patterns;
    mutable std::mutex pattern_mutex;
    
    // Statistics
    ErrorStats stats;
    std::mutex stats_mutex;
    
    // Background processing
    std::queue<ErrorEvent> error_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::thread processing_thread;
    
    // Configuration
    bool auto_reporting_enabled = false;
    bool user_consent_for_reporting = false;
    bool detailed_logging_enabled = true;
    bool stack_trace_enabled = true;
    
    // User interaction callbacks
    std::function<void(const ErrorEvent&)> user_notification_callback;
    std::function<bool(const std::string&)> user_consent_callback;
    std::function<void(const std::string&, const std::string&)> recovery_notification_callback;
    
    // Internal methods
    std::string generateErrorId();
    void processErrorQueue();
    void processError(const ErrorEvent& error);
    bool matchesPattern(const ErrorEvent& error, const ErrorPattern& pattern);
    void attemptAutoRecovery(const ErrorEvent& error);
    void updateErrorStats(const ErrorEvent& error);
    void rotateLogFile();
    std::string formatLogEntry(const ErrorEvent& error);
    std::string getStackTrace();
    void registerDefaultPatterns();
    void reportErrorToRemote(const ErrorEvent& error);
    
public:
    ErrorHandler();
    ~ErrorHandler();
    
    // Initialization and cleanup
    bool initialize(const std::string& log_file_path);
    void shutdown();
    
    // Error reporting
    void logError(const std::string& message, const std::string& context = "", 
                  ErrorCategory category = ErrorCategory::GENERAL);
    void logWarning(const std::string& message, const std::string& context = "", 
                    ErrorCategory category = ErrorCategory::GENERAL);
    void logCritical(const std::string& message, const std::string& context = "", 
                     ErrorCategory category = ErrorCategory::GENERAL);
    void logFatal(const std::string& message, const std::string& context = "", 
                  ErrorCategory category = ErrorCategory::GENERAL);
    void logInfo(const std::string& message, const std::string& context = "", 
                 ErrorCategory category = ErrorCategory::GENERAL);
    
    // Custom error reporting
    void reportError(ErrorSeverity severity, ErrorCategory category,
                    const std::string& message, const std::string& context = "",
                    const json& additional_data = json::object());
    
    // Error pattern management
    void registerErrorPattern(const ErrorPattern& pattern);
    void removeErrorPattern(const std::string& pattern_id);
    std::vector<ErrorPattern> getErrorPatterns() const;
    
    // Recovery and resolution
    bool attemptErrorRecovery(const std::string& error_id);
    void markErrorResolved(const std::string& error_id);
    std::vector<std::string> getSuggestedSolutions(ErrorCategory category);
    
    // User-friendly error handling
    std::string getUserFriendlyMessage(const ErrorEvent& error);
    std::vector<std::string> getRecoverySteps(const std::string& error_id);
    bool isErrorRecoverable(const std::string& error_id);
    
    // Error analysis and reporting
    std::vector<ErrorEvent> getRecentErrors(int count = 10);
    std::vector<ErrorEvent> getErrorsByCategory(ErrorCategory category);
    std::vector<ErrorEvent> getErrorsBySeverity(ErrorSeverity severity);
    ErrorEvent getErrorById(const std::string& error_id);
    
    // Statistics and metrics
    ErrorStats getErrorStats();
    void resetStats();
    std::string generateErrorReport();
    json exportErrorsAsJson();
    
    // Configuration
    void setAutoReportingEnabled(bool enabled);
    void setUserConsentForReporting(bool consent);
    void setDetailedLoggingEnabled(bool enabled);
    void setStackTraceEnabled(bool enabled);
    void setMaxHistorySize(int size);
    
    // Callbacks
    void setUserNotificationCallback(std::function<void(const ErrorEvent&)> callback);
    void setUserConsentCallback(std::function<bool(const std::string&)> callback);
    void setRecoveryNotificationCallback(std::function<void(const std::string&, const std::string&)> callback);
    
    // Maintenance
    bool selfTest();
    bool rotateLogFiles();
    bool cleanupOldLogs();
    
    // Error context management
    void pushContext(const std::string& context);
    void popContext();
    std::string getCurrentContext();
    
    // Batch operations
    void flushPendingErrors();
    void processErrorBatch(const std::vector<ErrorEvent>& errors);
    
    // Import/Export
    bool exportErrorLog(const std::string& export_path);
    bool importErrorLog(const std::string& import_path);
    
    // Real-time monitoring
    void startRealTimeMonitoring();
    void stopRealTimeMonitoring();
    bool isMonitoringActive() const;
    
    // Error suppression (for known non-critical issues)
    void suppressError(const std::string& pattern, std::chrono::minutes duration);
    void unsuppressError(const std::string& pattern);
    bool isErrorSuppressed(const std::string& message);
    
private:
    // Context stack for nested operations
    std::vector<std::string> context_stack;
    std::mutex context_mutex;
    
    // Error suppression
    std::map<std::string, std::chrono::system_clock::time_point> suppressed_errors;
    std::mutex suppression_mutex;
};

// Utility functions
std::string severityToString(ErrorSeverity severity);
std::string categoryToString(ErrorCategory category);
ErrorSeverity stringToSeverity(const std::string& severity_str);
ErrorCategory stringToCategory(const std::string& category_str);

// RAII context helper
class ErrorContext {
private:
    ErrorHandler* handler;
    
public:
    ErrorContext(ErrorHandler* handler, const std::string& context);
    ~ErrorContext();
};

// Macros for convenient error reporting
#define LOG_ERROR(handler, message) \
    handler->logError(message, __FILE__ ":" + std::to_string(__LINE__))

#define LOG_WARNING(handler, message) \
    handler->logWarning(message, __FILE__ ":" + std::to_string(__LINE__))

#define LOG_CRITICAL(handler, message) \
    handler->logCritical(message, __FILE__ ":" + std::to_string(__LINE__))

#define LOG_FATAL(handler, message) \
    handler->logFatal(message, __FILE__ ":" + std::to_string(__LINE__))

#define ERROR_CONTEXT(handler, context) \
    ErrorContext _error_context(handler, context)

#endif // ERROR_HANDLER_H