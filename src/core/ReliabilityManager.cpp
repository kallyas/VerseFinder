#include "ReliabilityManager.h"
#include "CrashRecoverySystem.h"
#include "ErrorHandler.h"
#include "HealthMonitor.h"
#include "BackupManager.h"
#include "EmergencyModeHandler.h"
#include <iostream>
#include <filesystem>
#include <fstream>

// Static members initialization
std::unique_ptr<ReliabilityManager> ReliabilityManager::instance = nullptr;
std::mutex ReliabilityManager::instance_mutex;

ReliabilityManager::ReliabilityManager() {
    // Initialize directories
    config_directory = "./config";
    backup_directory = "./backups";
    crash_recovery_directory = "./recovery";
    
    // Initialize health state
    overall_health.last_check = std::chrono::system_clock::now();
    overall_health.status_message = "System starting up";
}

ReliabilityManager::~ReliabilityManager() {
    stop();
}

ReliabilityManager& ReliabilityManager::getInstance() {
    std::lock_guard<std::mutex> lock(instance_mutex);
    if (!instance) {
        instance = std::unique_ptr<ReliabilityManager>(new ReliabilityManager());
    }
    return *instance;
}

void ReliabilityManager::destroyInstance() {
    std::lock_guard<std::mutex> lock(instance_mutex);
    if (instance) {
        instance->shutdown();
        instance.reset();
    }
}

bool ReliabilityManager::initialize(const std::string& app_directory) {
    if (is_initialized.load()) {
        return true; // Already initialized
    }
    
    try {
        // Create necessary directories
        std::filesystem::create_directories(config_directory);
        std::filesystem::create_directories(backup_directory);
        std::filesystem::create_directories(crash_recovery_directory);
        
        // Initialize core components
        crash_recovery = std::make_unique<CrashRecoverySystem>();
        error_handler = std::make_unique<ErrorHandler>();
        health_monitor = std::make_unique<HealthMonitor>();
        backup_manager = std::make_unique<BackupManager>();
        emergency_mode = std::make_unique<EmergencyModeHandler>();
        
        // Initialize each component
        if (!crash_recovery->initialize(crash_recovery_directory)) {
            reportError("Failed to initialize crash recovery system");
            return false;
        }
        
        if (!error_handler->initialize(config_directory + "/errors.log")) {
            reportError("Failed to initialize error handler");
            return false;
        }
        
        if (!health_monitor->initialize()) {
            reportError("Failed to initialize health monitor");
            return false;
        }
        
        if (!backup_manager->initialize(backup_directory)) {
            reportError("Failed to initialize backup manager");
            return false;
        }
        
        if (!emergency_mode->initialize()) {
            reportError("Failed to initialize emergency mode handler");
            return false;
        }
        
        is_initialized.store(true);
        
        // Update health status
        {
            std::lock_guard<std::mutex> lock(health_mutex);
            overall_health.is_healthy = true;
            overall_health.status_message = "System initialized successfully";
            overall_health.last_check = std::chrono::system_clock::now();
        }
        
        std::cout << "ReliabilityManager initialized successfully" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        reportCriticalError("Failed to initialize ReliabilityManager: " + std::string(e.what()));
        return false;
    }
}

bool ReliabilityManager::start() {
    if (!is_initialized.load()) {
        reportError("ReliabilityManager not initialized");
        return false;
    }
    
    if (is_running.load()) {
        return true; // Already running
    }
    
    try {
        // Start the auto-save thread
        if (auto_save_enabled.load()) {
            auto_save_thread = std::thread(&ReliabilityManager::autoSaveLoop, this);
        }
        
        // Start health monitoring
        if (health_monitor) {
            health_monitor->startMonitoring();
        }
        
        is_running.store(true);
        
        // Update reliability level to normal
        updateReliabilityLevel(ReliabilityLevel::NORMAL);
        
        std::cout << "ReliabilityManager started successfully" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        reportCriticalError("Failed to start ReliabilityManager: " + std::string(e.what()));
        return false;
    }
}

void ReliabilityManager::stop() {
    if (!is_running.load()) {
        return; // Already stopped
    }
    
    is_running.store(false);
    
    // Stop auto-save thread
    if (auto_save_thread.joinable()) {
        auto_save_cv.notify_all();
        auto_save_thread.join();
    }
    
    // Stop health monitoring
    if (health_monitor) {
        health_monitor->stopMonitoring();
    }
    
    // Perform final save
    saveCurrentState();
    
    std::cout << "ReliabilityManager stopped" << std::endl;
}

void ReliabilityManager::shutdown() {
    stop();
    
    // Shutdown all components
    if (crash_recovery) {
        crash_recovery->shutdown();
    }
    if (error_handler) {
        error_handler->shutdown();
    }
    if (health_monitor) {
        health_monitor->shutdown();
    }
    if (backup_manager) {
        backup_manager->shutdown();
    }
    if (emergency_mode) {
        emergency_mode->shutdown();
    }
    
    is_initialized.store(false);
    std::cout << "ReliabilityManager shutdown complete" << std::endl;
}

void ReliabilityManager::setAutoSaveInterval(std::chrono::seconds interval) {
    auto_save_interval = interval;
}

void ReliabilityManager::setConfigDirectory(const std::string& directory) {
    config_directory = directory;
}

void ReliabilityManager::setBackupDirectory(const std::string& directory) {
    backup_directory = directory;
}

void ReliabilityManager::enableAutoSave(bool enabled) {
    auto_save_enabled.store(enabled);
}

void ReliabilityManager::setReliabilityLevelChangedCallback(std::function<void()> callback) {
    on_reliability_level_changed = callback;
}

void ReliabilityManager::setStateManagementCallbacks(
    std::function<bool()> get_state,
    std::function<bool(const std::string&)> restore_state) {
    get_current_state_callback = get_state;
    restore_state_callback = restore_state;
}

bool ReliabilityManager::saveCurrentState() {
    if (!crash_recovery || !get_current_state_callback) {
        return false;
    }
    
    try {
        // Get current state from application
        if (get_current_state_callback()) {
            return crash_recovery->saveSessionState();
        }
        return false;
    } catch (const std::exception& e) {
        reportError("Failed to save current state: " + std::string(e.what()));
        return false;
    }
}

bool ReliabilityManager::restoreLastSession() {
    if (!crash_recovery || !restore_state_callback) {
        return false;
    }
    
    try {
        std::string session_data;
        if (crash_recovery->loadLastSession(session_data)) {
            return restore_state_callback(session_data);
        }
        return false;
    } catch (const std::exception& e) {
        reportError("Failed to restore last session: " + std::string(e.what()));
        return false;
    }
}

bool ReliabilityManager::hasRecoverableSession() {
    if (!crash_recovery) {
        return false;
    }
    return crash_recovery->hasRecoverableSession();
}

SystemHealth ReliabilityManager::getSystemHealth() {
    std::lock_guard<std::mutex> lock(health_mutex);
    return overall_health;
}

ReliabilityLevel ReliabilityManager::getCurrentReliabilityLevel() const {
    return current_level.load();
}

bool ReliabilityManager::isComponentHealthy(SystemComponent component) {
    if (!health_monitor) {
        return true; // Assume healthy if no monitor
    }
    return health_monitor->isComponentHealthy(component);
}

void ReliabilityManager::reportComponentIssue(SystemComponent component, const std::string& issue) {
    if (health_monitor) {
        health_monitor->reportComponentIssue(component, issue);
    }
    
    // Check if we need to degrade reliability level
    if (!performHealthCheck()) {
        updateReliabilityLevel(ReliabilityLevel::DEGRADED);
    }
}

void ReliabilityManager::reportComponentRecovery(SystemComponent component) {
    if (health_monitor) {
        health_monitor->reportComponentRecovery(component);
    }
    
    // Check if we can upgrade reliability level
    if (performHealthCheck() && current_level.load() != ReliabilityLevel::NORMAL) {
        updateReliabilityLevel(ReliabilityLevel::NORMAL);
    }
}

void ReliabilityManager::reportError(const std::string& error_message, const std::string& context) {
    if (error_handler) {
        error_handler->logError(error_message, context);
    }
    
    {
        std::lock_guard<std::mutex> lock(health_mutex);
        overall_health.error_count++;
    }
    
    std::cerr << "ERROR: " << error_message;
    if (!context.empty()) {
        std::cerr << " (Context: " << context << ")";
    }
    std::cerr << std::endl;
}

void ReliabilityManager::reportWarning(const std::string& warning_message, const std::string& context) {
    if (error_handler) {
        error_handler->logWarning(warning_message, context);
    }
    
    {
        std::lock_guard<std::mutex> lock(health_mutex);
        overall_health.warning_count++;
    }
    
    std::cout << "WARNING: " << warning_message;
    if (!context.empty()) {
        std::cout << " (Context: " << context << ")";
    }
    std::cout << std::endl;
}

void ReliabilityManager::reportCriticalError(const std::string& error_message, const std::string& context) {
    reportError(error_message, context);
    handleCriticalError(error_message);
}

bool ReliabilityManager::isInEmergencyMode() const {
    return current_level.load() == ReliabilityLevel::EMERGENCY;
}

bool ReliabilityManager::activateEmergencyMode(const std::string& reason) {
    if (!emergency_mode) {
        return false;
    }
    
    updateReliabilityLevel(ReliabilityLevel::EMERGENCY);
    return emergency_mode->activate(reason);
}

bool ReliabilityManager::exitEmergencyMode() {
    if (!emergency_mode) {
        return false;
    }
    
    if (emergency_mode->deactivate()) {
        updateReliabilityLevel(ReliabilityLevel::NORMAL);
        return true;
    }
    return false;
}

bool ReliabilityManager::createBackup(const std::string& backup_name) {
    if (!backup_manager) {
        return false;
    }
    return backup_manager->createBackup(backup_name);
}

bool ReliabilityManager::restoreFromBackup(const std::string& backup_name) {
    if (!backup_manager) {
        return false;
    }
    return backup_manager->restoreBackup(backup_name);
}

std::vector<std::string> ReliabilityManager::getAvailableBackups() {
    if (!backup_manager) {
        return {};
    }
    return backup_manager->getAvailableBackups();
}

bool ReliabilityManager::verifyDataIntegrity() {
    if (!backup_manager) {
        return true; // Assume integrity if no manager
    }
    return backup_manager->verifyIntegrity();
}

std::string ReliabilityManager::generateDiagnosticReport() {
    std::string report = "=== VerseFinder Reliability Diagnostic Report ===\n";
    
    // System health
    auto health = getSystemHealth();
    report += "System Health: ";
    report += (health.is_healthy ? "HEALTHY" : "UNHEALTHY");
    report += "\n";
    report += "Status: " + health.status_message + "\n";
    report += "Errors: " + std::to_string(health.error_count) + "\n";
    report += "Warnings: " + std::to_string(health.warning_count) + "\n";
    
    // Reliability level
    report += "Reliability Level: ";
    switch (current_level.load()) {
        case ReliabilityLevel::NORMAL: report += "NORMAL"; break;
        case ReliabilityLevel::DEGRADED: report += "DEGRADED"; break;
        case ReliabilityLevel::EMERGENCY: report += "EMERGENCY"; break;
        case ReliabilityLevel::CRITICAL: report += "CRITICAL"; break;
    }
    report += "\n";
    
    // Component status
    if (health_monitor) {
        report += health_monitor->generateReport();
    }
    
    // Backup status
    if (backup_manager) {
        report += backup_manager->generateReport();
    }
    
    return report;
}

bool ReliabilityManager::performSelfDiagnostic() {
    bool all_good = true;
    
    // Check each component
    if (crash_recovery && !crash_recovery->selfTest()) {
        reportError("Crash recovery system self-test failed");
        all_good = false;
    }
    
    if (error_handler && !error_handler->selfTest()) {
        reportError("Error handler self-test failed");
        all_good = false;
    }
    
    if (health_monitor && !health_monitor->selfTest()) {
        reportError("Health monitor self-test failed");
        all_good = false;
    }
    
    if (backup_manager && !backup_manager->selfTest()) {
        reportError("Backup manager self-test failed");
        all_good = false;
    }
    
    if (emergency_mode && !emergency_mode->selfTest()) {
        reportError("Emergency mode self-test failed");
        all_good = false;
    }
    
    return all_good;
}

bool ReliabilityManager::cleanupOldFiles() {
    bool success = true;
    
    if (crash_recovery) {
        success &= crash_recovery->cleanupOldFiles();
    }
    
    if (backup_manager) {
        success &= backup_manager->cleanupOldBackups();
    }
    
    if (error_handler) {
        success &= error_handler->rotateLogFiles();
    }
    
    return success;
}

void ReliabilityManager::optimizePerformance() {
    // Clean up old files
    cleanupOldFiles();
    
    // Optimize components
    if (health_monitor) {
        health_monitor->optimizePerformance();
    }
    
    if (backup_manager) {
        backup_manager->optimizeStorage();
    }
}

ReliabilityManager::ReliabilityStats ReliabilityManager::getReliabilityStats() {
    ReliabilityStats stats;
    
    {
        std::lock_guard<std::mutex> lock(health_mutex);
        stats.total_errors = overall_health.error_count;
        stats.total_warnings = overall_health.warning_count;
    }
    
    // Get stats from components
    if (crash_recovery) {
        auto recovery_stats = crash_recovery->getStats();
        stats.successful_recoveries = recovery_stats.successful_recoveries;
        stats.failed_recoveries = recovery_stats.failed_recoveries;
    }
    
    if (emergency_mode) {
        stats.emergency_mode_activations = emergency_mode->getActivationCount();
    }
    
    // Calculate stability rating
    int total_incidents = stats.total_errors + stats.failed_recoveries + stats.emergency_mode_activations;
    if (total_incidents == 0) {
        stats.stability_rating = 1.0;
    } else {
        stats.stability_rating = std::max(0.0, 1.0 - (total_incidents * 0.1));
    }
    
    return stats;
}

void ReliabilityManager::resetStats() {
    {
        std::lock_guard<std::mutex> lock(health_mutex);
        overall_health.error_count = 0;
        overall_health.warning_count = 0;
    }
    
    if (crash_recovery) {
        crash_recovery->resetStats();
    }
    
    if (emergency_mode) {
        emergency_mode->resetStats();
    }
}

// Private methods

void ReliabilityManager::autoSaveLoop() {
    while (is_running.load()) {
        std::unique_lock<std::mutex> lock(auto_save_mutex);
        if (auto_save_cv.wait_for(lock, auto_save_interval, [this] { return !is_running.load(); })) {
            break; // Shutdown requested
        }
        
        if (auto_save_enabled.load()) {
            saveCurrentState();
        }
    }
}

void ReliabilityManager::updateReliabilityLevel(ReliabilityLevel new_level) {
    ReliabilityLevel old_level = current_level.exchange(new_level);
    
    if (old_level != new_level) {
        std::cout << "Reliability level changed from " << static_cast<int>(old_level) 
                  << " to " << static_cast<int>(new_level) << std::endl;
        
        if (on_reliability_level_changed) {
            on_reliability_level_changed();
        }
    }
}

bool ReliabilityManager::performHealthCheck() {
    if (!health_monitor) {
        return true; // Assume healthy if no monitor
    }
    
    bool is_healthy = health_monitor->performHealthCheck();
    
    {
        std::lock_guard<std::mutex> lock(health_mutex);
        overall_health.is_healthy = is_healthy;
        overall_health.last_check = std::chrono::system_clock::now();
        
        if (is_healthy) {
            overall_health.status_message = "All systems operational";
        } else {
            overall_health.status_message = "System degradation detected";
        }
    }
    
    return is_healthy;
}

void ReliabilityManager::handleCriticalError(const std::string& error_message) {
    // Critical error handling sequence
    
    // 1. Try to save current state immediately
    saveCurrentState();
    
    // 2. Create emergency backup
    createBackup("emergency_" + std::to_string(std::time(nullptr)));
    
    // 3. Check if we should activate emergency mode
    if (current_level.load() != ReliabilityLevel::EMERGENCY) {
        activateEmergencyMode("Critical error: " + error_message);
    }
    
    // 4. Update reliability level to critical
    updateReliabilityLevel(ReliabilityLevel::CRITICAL);
}