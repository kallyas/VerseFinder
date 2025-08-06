#include "ErrorHandler.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <random>
#include <algorithm>
#include <regex>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <execinfo.h>
#include <cxxabi.h>
#endif

ErrorHandler::ErrorHandler() {
    stats = ErrorStats();
}

ErrorHandler::~ErrorHandler() {
    shutdown();
}

bool ErrorHandler::initialize(const std::string& log_file_path) {
    if (is_initialized.load()) {
        return true;
    }
    
    try {
        this->log_file_path = log_file_path;
        error_report_dir = std::filesystem::path(log_file_path).parent_path() / "error_reports";
        
        // Create directories
        std::filesystem::create_directories(std::filesystem::path(log_file_path).parent_path());
        std::filesystem::create_directories(error_report_dir);
        
        // Open log file
        log_file.open(log_file_path, std::ios::app);
        if (!log_file.is_open()) {
            std::cerr << "Failed to open log file: " << log_file_path << std::endl;
            return false;
        }
        
        // Start processing thread
        is_running.store(true);
        processing_thread = std::thread(&ErrorHandler::processErrorQueue, this);
        
        // Register default error patterns
        registerDefaultPatterns();
        
        is_initialized.store(true);
        
        // Log initialization
        logInfo("ErrorHandler initialized", "System startup");
        
        std::cout << "ErrorHandler initialized with log file: " << log_file_path << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize ErrorHandler: " << e.what() << std::endl;
        return false;
    }
}

void ErrorHandler::shutdown() {
    if (!is_initialized.load()) {
        return;
    }
    
    logInfo("ErrorHandler shutting down", "System shutdown");
    
    // Stop processing thread
    is_running.store(false);
    queue_cv.notify_all();
    
    if (processing_thread.joinable()) {
        processing_thread.join();
    }
    
    // Flush pending errors
    flushPendingErrors();
    
    // Close log file
    if (log_file.is_open()) {
        log_file.close();
    }
    
    is_initialized.store(false);
    std::cout << "ErrorHandler shutdown complete" << std::endl;
}

std::string ErrorHandler::generateErrorId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(100000, 999999);
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "ERR_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    ss << "_" << dis(gen);
    
    return ss.str();
}

void ErrorHandler::logError(const std::string& message, const std::string& context, ErrorCategory category) {
    reportError(ErrorSeverity::ERROR, category, message, context);
}

void ErrorHandler::logWarning(const std::string& message, const std::string& context, ErrorCategory category) {
    reportError(ErrorSeverity::WARNING, category, message, context);
}

void ErrorHandler::logCritical(const std::string& message, const std::string& context, ErrorCategory category) {
    reportError(ErrorSeverity::CRITICAL, category, message, context);
}

void ErrorHandler::logFatal(const std::string& message, const std::string& context, ErrorCategory category) {
    reportError(ErrorSeverity::FATAL, category, message, context);
}

void ErrorHandler::logInfo(const std::string& message, const std::string& context, ErrorCategory category) {
    reportError(ErrorSeverity::INFO, category, message, context);
}

void ErrorHandler::reportError(ErrorSeverity severity, ErrorCategory category,
                              const std::string& message, const std::string& context,
                              const json& additional_data) {
    if (!is_initialized.load()) {
        // Fall back to console output
        std::cerr << "[" << severityToString(severity) << "] " << message << std::endl;
        return;
    }
    
    try {
        ErrorEvent error;
        error.id = generateErrorId();
        error.severity = severity;
        error.category = category;
        error.message = message;
        error.context = context + " | " + getCurrentContext();
        error.timestamp = std::chrono::system_clock::now();
        error.additional_data = additional_data;
        
        // Get stack trace for errors and above
        if (stack_trace_enabled && severity >= ErrorSeverity::ERROR) {
            error.stack_trace = getStackTrace();
        }
        
        // Add to queue for processing
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            error_queue.push(error);
        }
        queue_cv.notify_one();
        
        // For critical and fatal errors, process immediately
        if (severity >= ErrorSeverity::CRITICAL) {
            flushPendingErrors();
        }
        
    } catch (const std::exception& e) {
        // Error in error handling - fall back to console
        std::cerr << "Error in ErrorHandler: " << e.what() << std::endl;
        std::cerr << "[" << severityToString(severity) << "] " << message << std::endl;
    }
}

void ErrorHandler::processErrorQueue() {
    while (is_running.load()) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        queue_cv.wait(lock, [this] { return !error_queue.empty() || !is_running.load(); });
        
        while (!error_queue.empty()) {
            ErrorEvent error = error_queue.front();
            error_queue.pop();
            lock.unlock();
            
            // Process the error
            processError(error);
            
            lock.lock();
        }
    }
}

void ErrorHandler::processError(const ErrorEvent& error) {
    // Add to history
    {
        std::lock_guard<std::mutex> lock(error_mutex);
        error_history.push_back(error);
        
        // Maintain history size limit
        if (error_history.size() > static_cast<size_t>(max_history_size)) {
            error_history.erase(error_history.begin());
        }
    }
    
    // Write to log file
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        if (log_file.is_open()) {
            log_file << formatLogEntry(error) << std::endl;
            log_file.flush();
        }
    }
    
    // Update statistics
    updateErrorStats(error);
    
    // Check for error patterns and attempt recovery
    attemptAutoRecovery(error);
    
    // Notify user if callback is set and severity is high enough
    if (user_notification_callback && error.severity >= ErrorSeverity::ERROR) {
        user_notification_callback(error);
    }
    
    // Auto-report if enabled and user consented
    if (auto_reporting_enabled && user_consent_for_reporting && 
        error.severity >= ErrorSeverity::ERROR) {
        reportErrorToRemote(error);
    }
    
    // Console output for immediate visibility
    if (error.severity >= ErrorSeverity::WARNING) {
        std::cerr << "[" << severityToString(error.severity) << "] " 
                  << error.message << std::endl;
    }
}

void ErrorHandler::attemptAutoRecovery(const ErrorEvent& error) {
    std::lock_guard<std::mutex> lock(pattern_mutex);
    
    for (const auto& pattern : error_patterns) {
        if (matchesPattern(error, pattern) && pattern.auto_recoverable) {
            try {
                if (pattern.recovery_action && pattern.recovery_action()) {
                    logInfo("Auto-recovery successful for error: " + error.id, 
                           "Pattern: " + pattern.pattern_id);
                    
                    if (recovery_notification_callback) {
                        recovery_notification_callback(error.id, "Auto-recovery successful");
                    }
                    
                    {
                        std::lock_guard<std::mutex> stats_lock(stats_mutex);
                        stats.auto_recovered++;
                        stats.resolved_errors++;
                    }
                }
            } catch (const std::exception& e) {
                logError("Auto-recovery failed for error: " + error.id, 
                        "Recovery exception: " + std::string(e.what()));
            }
            break; // Only try first matching pattern
        }
    }
}

bool ErrorHandler::matchesPattern(const ErrorEvent& error, const ErrorPattern& pattern) {
    try {
        // Simple regex matching for now
        std::regex pattern_regex(pattern.message_pattern);
        return std::regex_search(error.message, pattern_regex) && 
               error.category == pattern.category;
    } catch (const std::exception&) {
        return false; // Invalid regex pattern
    }
}

void ErrorHandler::updateErrorStats(const ErrorEvent& error) {
    std::lock_guard<std::mutex> lock(stats_mutex);
    
    switch (error.severity) {
        case ErrorSeverity::INFO:
            break; // Don't count info messages
        case ErrorSeverity::WARNING:
            stats.total_warnings++;
            break;
        case ErrorSeverity::ERROR:
            stats.total_errors++;
            break;
        case ErrorSeverity::CRITICAL:
            stats.total_critical++;
            break;
        case ErrorSeverity::FATAL:
            stats.total_fatal++;
            break;
    }
    
    // Calculate error rate (errors per hour)
    // This is a simplified calculation
    static auto start_time = std::chrono::system_clock::now();
    auto elapsed = std::chrono::system_clock::now() - start_time;
    auto hours = std::chrono::duration_cast<std::chrono::hours>(elapsed).count();
    
    if (hours > 0) {
        stats.error_rate = static_cast<double>(stats.total_errors + stats.total_critical + stats.total_fatal) / hours;
    }
}

std::string ErrorHandler::formatLogEntry(const ErrorEvent& error) {
    std::stringstream ss;
    
    // Timestamp
    auto time_t = std::chrono::system_clock::to_time_t(error.timestamp);
    ss << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "] ";
    
    // Severity and category
    ss << "[" << severityToString(error.severity) << "] ";
    ss << "[" << categoryToString(error.category) << "] ";
    
    // Error ID
    ss << "[" << error.id << "] ";
    
    // Message
    ss << error.message;
    
    // Context
    if (!error.context.empty()) {
        ss << " | Context: " << error.context;
    }
    
    // Additional data
    if (!error.additional_data.empty()) {
        ss << " | Data: " << error.additional_data.dump();
    }
    
    // Stack trace
    if (!error.stack_trace.empty()) {
        ss << " | Stack: " << error.stack_trace;
    }
    
    return ss.str();
}

std::string ErrorHandler::getStackTrace() {
    if (!stack_trace_enabled) {
        return "";
    }
    
    std::string trace;
    
#ifdef _WIN32
    // Windows stack trace implementation
    void* stack[100];
    unsigned short frames = CaptureStackBackTrace(0, 100, stack, NULL);
    
    for (unsigned short i = 0; i < frames; ++i) {
        trace += std::to_string(reinterpret_cast<uintptr_t>(stack[i])) + " ";
    }
    
#elif defined(__linux__) || defined(__APPLE__)
    // Linux/macOS stack trace implementation
    void* array[100];
    size_t size = backtrace(array, 100);
    char** strings = backtrace_symbols(array, size);
    
    if (strings) {
        for (size_t i = 0; i < size; ++i) {
            trace += std::string(strings[i]) + "\n";
        }
        free(strings);
    }
#endif
    
    return trace;
}

void ErrorHandler::registerErrorPattern(const ErrorPattern& pattern) {
    std::lock_guard<std::mutex> lock(pattern_mutex);
    error_patterns.push_back(pattern);
}

void ErrorHandler::removeErrorPattern(const std::string& pattern_id) {
    std::lock_guard<std::mutex> lock(pattern_mutex);
    error_patterns.erase(
        std::remove_if(error_patterns.begin(), error_patterns.end(),
                      [&pattern_id](const ErrorPattern& p) { return p.pattern_id == pattern_id; }),
        error_patterns.end());
}

void ErrorHandler::registerDefaultPatterns() {
    // File not found pattern
    ErrorPattern file_not_found;
    file_not_found.pattern_id = "file_not_found";
    file_not_found.message_pattern = ".*[Ff]ile not found.*|.*[Cc]ould not open.*";
    file_not_found.category = ErrorCategory::FILE_IO;
    file_not_found.user_friendly_message = "A required file is missing. Please check if all necessary files are present.";
    file_not_found.suggested_solution = "Verify file paths and ensure all required files are installed.";
    registerErrorPattern(file_not_found);
    
    // Memory allocation pattern
    ErrorPattern memory_error;
    memory_error.pattern_id = "memory_allocation";
    memory_error.message_pattern = ".*[Bb]ad alloc.*|.*[Oo]ut of memory.*";
    memory_error.category = ErrorCategory::MEMORY;
    memory_error.user_friendly_message = "The application is running low on memory.";
    memory_error.suggested_solution = "Close other applications to free up memory, or restart VerseFinder.";
    registerErrorPattern(memory_error);
    
    // Network connectivity pattern
    ErrorPattern network_error;
    network_error.pattern_id = "network_connectivity";
    network_error.message_pattern = ".*[Nn]etwork.*|.*[Cc]onnection.*failed.*";
    network_error.category = ErrorCategory::NETWORK;
    network_error.user_friendly_message = "Network connection is unavailable.";
    network_error.suggested_solution = "Check your internet connection and try again.";
    registerErrorPattern(network_error);
    
    // Search engine pattern with auto-recovery
    ErrorPattern search_error;
    search_error.pattern_id = "search_engine_error";
    search_error.message_pattern = ".*[Ss]earch.*failed.*|.*[Ii]ndex.*corrupt.*";
    search_error.category = ErrorCategory::SEARCH_ENGINE;
    search_error.user_friendly_message = "Search functionality encountered an issue.";
    search_error.suggested_solution = "The search index will be rebuilt automatically.";
    search_error.auto_recoverable = true;
    search_error.recovery_action = []() {
        // In a real implementation, this would trigger search index rebuild
        std::cout << "Auto-recovery: Rebuilding search index..." << std::endl;
        return true;
    };
    registerErrorPattern(search_error);
}

std::vector<ErrorPattern> ErrorHandler::getErrorPatterns() const {
    std::lock_guard<std::mutex> lock(pattern_mutex);
    return error_patterns;
}

bool ErrorHandler::attemptErrorRecovery(const std::string& error_id) {
    ErrorEvent error = getErrorById(error_id);
    if (error.id.empty()) {
        return false;
    }
    
    attemptAutoRecovery(error);
    return true;
}

void ErrorHandler::markErrorResolved(const std::string& error_id) {
    std::lock_guard<std::mutex> lock(error_mutex);
    
    for (auto& error : error_history) {
        if (error.id == error_id) {
            // Mark as resolved in additional data
            error.additional_data["resolved"] = true;
            error.additional_data["resolution_time"] = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            
            {
                std::lock_guard<std::mutex> stats_lock(stats_mutex);
                stats.resolved_errors++;
            }
            break;
        }
    }
}

std::vector<std::string> ErrorHandler::getSuggestedSolutions(ErrorCategory category) {
    std::vector<std::string> solutions;
    
    std::lock_guard<std::mutex> lock(pattern_mutex);
    for (const auto& pattern : error_patterns) {
        if (pattern.category == category && !pattern.suggested_solution.empty()) {
            solutions.push_back(pattern.suggested_solution);
        }
    }
    
    return solutions;
}

std::string ErrorHandler::getUserFriendlyMessage(const ErrorEvent& error) {
    std::lock_guard<std::mutex> lock(pattern_mutex);
    
    for (const auto& pattern : error_patterns) {
        if (matchesPattern(error, pattern) && !pattern.user_friendly_message.empty()) {
            return pattern.user_friendly_message;
        }
    }
    
    // Default user-friendly messages based on severity
    switch (error.severity) {
        case ErrorSeverity::WARNING:
            return "A minor issue was detected but operation can continue.";
        case ErrorSeverity::ERROR:
            return "An error occurred that may affect some functionality.";
        case ErrorSeverity::CRITICAL:
            return "A serious error occurred that requires attention.";
        case ErrorSeverity::FATAL:
            return "A critical error occurred that may require restarting the application.";
        default:
            return error.message;
    }
}

std::vector<std::string> ErrorHandler::getRecoverySteps(const std::string& error_id) {
    ErrorEvent error = getErrorById(error_id);
    if (error.id.empty()) {
        return {};
    }
    
    std::vector<std::string> steps;
    
    // Category-specific recovery steps
    switch (error.category) {
        case ErrorCategory::FILE_IO:
            steps.push_back("Check if the file exists and is accessible");
            steps.push_back("Verify file permissions");
            steps.push_back("Try restarting the application");
            break;
            
        case ErrorCategory::MEMORY:
            steps.push_back("Close unnecessary applications");
            steps.push_back("Restart VerseFinder");
            steps.push_back("Check available system memory");
            break;
            
        case ErrorCategory::NETWORK:
            steps.push_back("Check internet connection");
            steps.push_back("Verify firewall settings");
            steps.push_back("Try again in a few moments");
            break;
            
        case ErrorCategory::SEARCH_ENGINE:
            steps.push_back("Clear search cache");
            steps.push_back("Rebuild search index");
            steps.push_back("Restart the application");
            break;
            
        default:
            steps.push_back("Try restarting the application");
            steps.push_back("Check system resources");
            steps.push_back("Contact support if problem persists");
            break;
    }
    
    return steps;
}

bool ErrorHandler::isErrorRecoverable(const std::string& error_id) {
    ErrorEvent error = getErrorById(error_id);
    if (error.id.empty()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(pattern_mutex);
    for (const auto& pattern : error_patterns) {
        if (matchesPattern(error, pattern)) {
            return pattern.auto_recoverable;
        }
    }
    
    // Most errors are potentially recoverable except fatal ones
    return error.severity != ErrorSeverity::FATAL;
}

std::vector<ErrorEvent> ErrorHandler::getRecentErrors(int count) {
    std::lock_guard<std::mutex> lock(error_mutex);
    
    std::vector<ErrorEvent> recent;
    int start = std::max(0, static_cast<int>(error_history.size()) - count);
    
    for (int i = start; i < static_cast<int>(error_history.size()); ++i) {
        recent.push_back(error_history[i]);
    }
    
    return recent;
}

std::vector<ErrorEvent> ErrorHandler::getErrorsByCategory(ErrorCategory category) {
    std::lock_guard<std::mutex> lock(error_mutex);
    
    std::vector<ErrorEvent> filtered;
    for (const auto& error : error_history) {
        if (error.category == category) {
            filtered.push_back(error);
        }
    }
    
    return filtered;
}

std::vector<ErrorEvent> ErrorHandler::getErrorsBySeverity(ErrorSeverity severity) {
    std::lock_guard<std::mutex> lock(error_mutex);
    
    std::vector<ErrorEvent> filtered;
    for (const auto& error : error_history) {
        if (error.severity == severity) {
            filtered.push_back(error);
        }
    }
    
    return filtered;
}

ErrorEvent ErrorHandler::getErrorById(const std::string& error_id) {
    std::lock_guard<std::mutex> lock(error_mutex);
    
    for (const auto& error : error_history) {
        if (error.id == error_id) {
            return error;
        }
    }
    
    return ErrorEvent(); // Return empty event if not found
}

ErrorStats ErrorHandler::getErrorStats() {
    std::lock_guard<std::mutex> lock(stats_mutex);
    return stats;
}

void ErrorHandler::resetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex);
    stats = ErrorStats();
}

std::string ErrorHandler::generateErrorReport() {
    std::stringstream report;
    
    report << "\n=== Error Handler Report ===\n";
    
    auto stats = getErrorStats();
    report << "Total Errors: " << stats.total_errors << "\n";
    report << "Total Warnings: " << stats.total_warnings << "\n";
    report << "Critical Errors: " << stats.total_critical << "\n";
    report << "Fatal Errors: " << stats.total_fatal << "\n";
    report << "Resolved Errors: " << stats.resolved_errors << "\n";
    report << "Auto-Recovered: " << stats.auto_recovered << "\n";
    report << "Error Rate: " << std::fixed << std::setprecision(2) << stats.error_rate << " errors/hour\n";
    
    report << "\nRecent Errors:\n";
    auto recent = getRecentErrors(5);
    for (const auto& error : recent) {
        auto time_t = std::chrono::system_clock::to_time_t(error.timestamp);
        report << "  " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") 
               << " [" << severityToString(error.severity) << "] " << error.message << "\n";
    }
    
    return report.str();
}

json ErrorHandler::exportErrorsAsJson() {
    std::lock_guard<std::mutex> lock(error_mutex);
    
    json export_data;
    export_data["export_timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    export_data["stats"] = {
        {"total_errors", stats.total_errors},
        {"total_warnings", stats.total_warnings},
        {"total_critical", stats.total_critical},
        {"total_fatal", stats.total_fatal},
        {"resolved_errors", stats.resolved_errors},
        {"auto_recovered", stats.auto_recovered},
        {"error_rate", stats.error_rate}
    };
    
    json errors_array = json::array();
    for (const auto& error : error_history) {
        json error_json = {
            {"id", error.id},
            {"severity", static_cast<int>(error.severity)},
            {"category", static_cast<int>(error.category)},
            {"message", error.message},
            {"context", error.context},
            {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(
                error.timestamp.time_since_epoch()).count()},
            {"additional_data", error.additional_data}
        };
        errors_array.push_back(error_json);
    }
    
    export_data["errors"] = errors_array;
    return export_data;
}

// Configuration methods
void ErrorHandler::setAutoReportingEnabled(bool enabled) {
    auto_reporting_enabled = enabled;
}

void ErrorHandler::setUserConsentForReporting(bool consent) {
    user_consent_for_reporting = consent;
}

void ErrorHandler::setDetailedLoggingEnabled(bool enabled) {
    detailed_logging_enabled = enabled;
}

void ErrorHandler::setStackTraceEnabled(bool enabled) {
    stack_trace_enabled = enabled;
}

void ErrorHandler::setMaxHistorySize(int size) {
    max_history_size = size;
}

// Callback setters
void ErrorHandler::setUserNotificationCallback(std::function<void(const ErrorEvent&)> callback) {
    user_notification_callback = callback;
}

void ErrorHandler::setUserConsentCallback(std::function<bool(const std::string&)> callback) {
    user_consent_callback = callback;
}

void ErrorHandler::setRecoveryNotificationCallback(std::function<void(const std::string&, const std::string&)> callback) {
    recovery_notification_callback = callback;
}

bool ErrorHandler::selfTest() {
    try {
        // Test error reporting
        reportError(ErrorSeverity::INFO, ErrorCategory::GENERAL, "Self-test message", "Self-test context");
        
        // Test pattern matching
        if (error_patterns.empty()) {
            return false;
        }
        
        // Test log file writing
        if (!log_file.is_open()) {
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "ErrorHandler self-test failed: " << e.what() << std::endl;
        return false;
    }
}

bool ErrorHandler::rotateLogFiles() {
    try {
        if (log_file.is_open()) {
            log_file.close();
        }
        
        // Create backup of current log
        std::string backup_path = log_file_path + ".backup";
        if (std::filesystem::exists(log_file_path)) {
            std::filesystem::rename(log_file_path, backup_path);
        }
        
        // Reopen log file
        log_file.open(log_file_path, std::ios::app);
        return log_file.is_open();
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to rotate log files: " << e.what() << std::endl;
        return false;
    }
}

bool ErrorHandler::cleanupOldLogs() {
    try {
        auto now = std::chrono::system_clock::now();
        auto retention_period = std::chrono::hours(24 * 30); // 30 days
        
        auto parent_dir = std::filesystem::path(log_file_path).parent_path();
        for (const auto& entry : std::filesystem::directory_iterator(parent_dir)) {
            if (entry.is_regular_file() && 
                (entry.path().extension() == ".log" || entry.path().filename().string().find("backup") != std::string::npos)) {
                
                auto file_time = std::filesystem::last_write_time(entry);
                auto file_age = now - std::chrono::system_clock::time_point(
                    std::chrono::duration_cast<std::chrono::system_clock::duration>(
                        file_time.time_since_epoch()));
                
                if (file_age > retention_period) {
                    std::filesystem::remove(entry.path());
                }
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to cleanup old logs: " << e.what() << std::endl;
        return false;
    }
}

// Context management
void ErrorHandler::pushContext(const std::string& context) {
    std::lock_guard<std::mutex> lock(context_mutex);
    context_stack.push_back(context);
}

void ErrorHandler::popContext() {
    std::lock_guard<std::mutex> lock(context_mutex);
    if (!context_stack.empty()) {
        context_stack.pop_back();
    }
}

std::string ErrorHandler::getCurrentContext() {
    std::lock_guard<std::mutex> lock(context_mutex);
    
    if (context_stack.empty()) {
        return "";
    }
    
    std::string full_context;
    for (size_t i = 0; i < context_stack.size(); ++i) {
        if (i > 0) full_context += " -> ";
        full_context += context_stack[i];
    }
    
    return full_context;
}

void ErrorHandler::flushPendingErrors() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    // This forces the processing thread to wake up and process all pending errors
    queue_cv.notify_all();
}

void ErrorHandler::processErrorBatch(const std::vector<ErrorEvent>& errors) {
    for (const auto& error : errors) {
        processError(error);
    }
}

void ErrorHandler::reportErrorToRemote(const ErrorEvent& error) {
    // This would implement remote error reporting
    // For now, just log that we would report it
    logInfo("Error marked for remote reporting: " + error.id, "Auto-reporting");
}

// Utility functions
std::string severityToString(ErrorSeverity severity) {
    switch (severity) {
        case ErrorSeverity::INFO: return "INFO";
        case ErrorSeverity::WARNING: return "WARNING";
        case ErrorSeverity::ERROR: return "ERROR";
        case ErrorSeverity::CRITICAL: return "CRITICAL";
        case ErrorSeverity::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

std::string categoryToString(ErrorCategory category) {
    switch (category) {
        case ErrorCategory::GENERAL: return "GENERAL";
        case ErrorCategory::SEARCH_ENGINE: return "SEARCH_ENGINE";
        case ErrorCategory::UI_SYSTEM: return "UI_SYSTEM";
        case ErrorCategory::DATA_STORAGE: return "DATA_STORAGE";
        case ErrorCategory::NETWORK: return "NETWORK";
        case ErrorCategory::MEMORY: return "MEMORY";
        case ErrorCategory::FILE_IO: return "FILE_IO";
        case ErrorCategory::TRANSLATION: return "TRANSLATION";
        case ErrorCategory::PRESENTATION: return "PRESENTATION";
        case ErrorCategory::CRASH_RECOVERY: return "CRASH_RECOVERY";
        case ErrorCategory::BACKUP_SYSTEM: return "BACKUP_SYSTEM";
        case ErrorCategory::HEALTH_MONITORING: return "HEALTH_MONITORING";
        default: return "UNKNOWN";
    }
}

ErrorSeverity stringToSeverity(const std::string& severity_str) {
    if (severity_str == "INFO") return ErrorSeverity::INFO;
    if (severity_str == "WARNING") return ErrorSeverity::WARNING;
    if (severity_str == "ERROR") return ErrorSeverity::ERROR;
    if (severity_str == "CRITICAL") return ErrorSeverity::CRITICAL;
    if (severity_str == "FATAL") return ErrorSeverity::FATAL;
    return ErrorSeverity::ERROR;
}

ErrorCategory stringToCategory(const std::string& category_str) {
    if (category_str == "SEARCH_ENGINE") return ErrorCategory::SEARCH_ENGINE;
    if (category_str == "UI_SYSTEM") return ErrorCategory::UI_SYSTEM;
    if (category_str == "DATA_STORAGE") return ErrorCategory::DATA_STORAGE;
    if (category_str == "NETWORK") return ErrorCategory::NETWORK;
    if (category_str == "MEMORY") return ErrorCategory::MEMORY;
    if (category_str == "FILE_IO") return ErrorCategory::FILE_IO;
    if (category_str == "TRANSLATION") return ErrorCategory::TRANSLATION;
    if (category_str == "PRESENTATION") return ErrorCategory::PRESENTATION;
    if (category_str == "CRASH_RECOVERY") return ErrorCategory::CRASH_RECOVERY;
    if (category_str == "BACKUP_SYSTEM") return ErrorCategory::BACKUP_SYSTEM;
    if (category_str == "HEALTH_MONITORING") return ErrorCategory::HEALTH_MONITORING;
    return ErrorCategory::GENERAL;
}

// ErrorContext RAII helper
ErrorContext::ErrorContext(ErrorHandler* handler, const std::string& context) 
    : handler(handler) {
    if (handler) {
        handler->pushContext(context);
    }
}

ErrorContext::~ErrorContext() {
    if (handler) {
        handler->popContext();
    }
}