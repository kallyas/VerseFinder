#ifndef RELIABILITY_MANAGER_H
#define RELIABILITY_MANAGER_H

#include <string>
#include <memory>
#include <chrono>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

// Forward declarations
class CrashRecoverySystem;
class ErrorHandler;
class HealthMonitor;
class BackupManager;
class EmergencyModeHandler;

enum class ReliabilityLevel {
    NORMAL = 0,
    DEGRADED = 1,
    EMERGENCY = 2,
    CRITICAL = 3
};

enum class SystemComponent {
    CORE_ENGINE,
    UI_SYSTEM,
    PRESENTATION_MODE,
    TRANSLATION_SYSTEM,
    SEARCH_ENGINE,
    DATA_STORAGE,
    NETWORK_CONNECTIVITY,
    MEMORY_MANAGEMENT,
    FILE_SYSTEM
};

struct SystemHealth {
    bool is_healthy = true;
    std::string status_message;
    double cpu_usage = 0.0;
    double memory_usage = 0.0;
    double disk_usage = 0.0;
    std::chrono::system_clock::time_point last_check;
    int error_count = 0;
    int warning_count = 0;
};

class ReliabilityManager {
private:
    // Singleton instance
    static std::unique_ptr<ReliabilityManager> instance;
    static std::mutex instance_mutex;
    
    // Core reliability components
    std::unique_ptr<CrashRecoverySystem> crash_recovery;
    std::unique_ptr<ErrorHandler> error_handler;
    std::unique_ptr<HealthMonitor> health_monitor;
    std::unique_ptr<BackupManager> backup_manager;
    std::unique_ptr<EmergencyModeHandler> emergency_mode;
    
    // State management
    std::atomic<ReliabilityLevel> current_level{ReliabilityLevel::NORMAL};
    std::atomic<bool> is_initialized{false};
    std::atomic<bool> is_running{false};
    std::atomic<bool> auto_save_enabled{true};
    
    // Auto-save system
    std::thread auto_save_thread;
    std::mutex auto_save_mutex;
    std::condition_variable auto_save_cv;
    std::chrono::seconds auto_save_interval{30};
    
    // System health tracking
    SystemHealth overall_health;
    std::mutex health_mutex;
    
    // Configuration
    std::string config_directory;
    std::string backup_directory;
    std::string crash_recovery_directory;
    
    // Callbacks for application integration
    std::function<void()> on_reliability_level_changed;
    std::function<bool()> get_current_state_callback;
    std::function<bool(const std::string&)> restore_state_callback;
    
    // Private constructor for singleton
    ReliabilityManager();
    
    // Internal methods
    void autoSaveLoop();
    void updateReliabilityLevel(ReliabilityLevel new_level);
    bool performHealthCheck();
    void handleCriticalError(const std::string& error_message);
    
public:
    // Singleton access
    static ReliabilityManager& getInstance();
    static void destroyInstance();
    
    // Destructor
    ~ReliabilityManager();
    
    // Core lifecycle methods
    bool initialize(const std::string& app_directory);
    bool start();
    void stop();
    void shutdown();
    
    // Configuration methods
    void setAutoSaveInterval(std::chrono::seconds interval);
    void setConfigDirectory(const std::string& directory);
    void setBackupDirectory(const std::string& directory);
    void enableAutoSave(bool enabled);
    
    // Callback registration
    void setReliabilityLevelChangedCallback(std::function<void()> callback);
    void setStateManagementCallbacks(
        std::function<bool()> get_state,
        std::function<bool(const std::string&)> restore_state
    );
    
    // State management
    bool saveCurrentState();
    bool restoreLastSession();
    bool hasRecoverableSession();
    
    // System health and monitoring
    SystemHealth getSystemHealth();
    ReliabilityLevel getCurrentReliabilityLevel() const;
    bool isComponentHealthy(SystemComponent component);
    void reportComponentIssue(SystemComponent component, const std::string& issue);
    void reportComponentRecovery(SystemComponent component);
    
    // Error handling
    void reportError(const std::string& error_message, const std::string& context = "");
    void reportWarning(const std::string& warning_message, const std::string& context = "");
    void reportCriticalError(const std::string& error_message, const std::string& context = "");
    
    // Emergency mode
    bool isInEmergencyMode() const;
    bool activateEmergencyMode(const std::string& reason);
    bool exitEmergencyMode();
    
    // Backup and recovery
    bool createBackup(const std::string& backup_name = "");
    bool restoreFromBackup(const std::string& backup_name = "");
    std::vector<std::string> getAvailableBackups();
    bool verifyDataIntegrity();
    
    // Diagnostics and maintenance
    std::string generateDiagnosticReport();
    bool performSelfDiagnostic();
    bool cleanupOldFiles();
    void optimizePerformance();
    
    // Component access (for advanced operations)
    CrashRecoverySystem* getCrashRecovery() { return crash_recovery.get(); }
    ErrorHandler* getErrorHandler() { return error_handler.get(); }
    HealthMonitor* getHealthMonitor() { return health_monitor.get(); }
    BackupManager* getBackupManager() { return backup_manager.get(); }
    EmergencyModeHandler* getEmergencyMode() { return emergency_mode.get(); }
    
    // Statistics and metrics
    struct ReliabilityStats {
        int total_errors = 0;
        int total_warnings = 0;
        int successful_recoveries = 0;
        int failed_recoveries = 0;
        int emergency_mode_activations = 0;
        std::chrono::seconds total_uptime{0};
        std::chrono::seconds average_recovery_time{0};
        double stability_rating = 1.0; // 0.0 to 1.0
    };
    
    ReliabilityStats getReliabilityStats();
    void resetStats();
    
    // Non-copyable and non-movable
    ReliabilityManager(const ReliabilityManager&) = delete;
    ReliabilityManager& operator=(const ReliabilityManager&) = delete;
    ReliabilityManager(ReliabilityManager&&) = delete;
    ReliabilityManager& operator=(ReliabilityManager&&) = delete;
};

#endif // RELIABILITY_MANAGER_H