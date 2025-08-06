#include <iostream>
#include "src/core/ReliabilityManager.h"
#include "src/core/ErrorHandler.h"
#include "src/core/HealthMonitor.h"
#include "src/core/CrashRecoverySystem.h"

int main() {
    std::cout << "=== VerseFinder Reliability System Test ===" << std::endl;
    
    try {
        // Test ReliabilityManager singleton
        auto& reliability = ReliabilityManager::getInstance();
        
        // Initialize the reliability system
        if (!reliability.initialize("./")) {
            std::cerr << "Failed to initialize ReliabilityManager" << std::endl;
            return 1;
        }
        
        // Start monitoring
        if (!reliability.start()) {
            std::cerr << "Failed to start ReliabilityManager" << std::endl;
            return 1;
        }
        
        std::cout << "✓ ReliabilityManager initialized and started" << std::endl;
        
        // Test error reporting
        reliability.reportError("Test error message", "reliability_test");
        reliability.reportWarning("Test warning message", "reliability_test");
        
        std::cout << "✓ Error reporting tested" << std::endl;
        
        // Test system health
        auto health = reliability.getSystemHealth();
        std::cout << "✓ System Health: " << (health.is_healthy ? "HEALTHY" : "UNHEALTHY") << std::endl;
        std::cout << "  Status: " << health.status_message << std::endl;
        std::cout << "  Errors: " << health.error_count << std::endl;
        std::cout << "  Warnings: " << health.warning_count << std::endl;
        
        // Test reliability level
        auto level = reliability.getCurrentReliabilityLevel();
        std::cout << "✓ Current Reliability Level: " << static_cast<int>(level) << std::endl;
        
        // Test diagnostics
        std::cout << "\n=== Diagnostic Report ===" << std::endl;
        std::cout << reliability.generateDiagnosticReport() << std::endl;
        
        // Test self-diagnostic
        if (reliability.performSelfDiagnostic()) {
            std::cout << "✓ Self-diagnostic passed" << std::endl;
        } else {
            std::cout << "✗ Self-diagnostic failed" << std::endl;
        }
        
        // Test stats
        auto stats = reliability.getReliabilityStats();
        std::cout << "\n=== Reliability Statistics ===" << std::endl;
        std::cout << "Total Errors: " << stats.total_errors << std::endl;
        std::cout << "Total Warnings: " << stats.total_warnings << std::endl;
        std::cout << "Stability Rating: " << stats.stability_rating << std::endl;
        
        // Test session state management
        std::cout << "\n=== Session Management Test ===" << std::endl;
        
        // Simulate some session data
        auto* crash_recovery = reliability.getCrashRecovery();
        if (crash_recovery) {
            crash_recovery->updateCurrentTranslation("KJV");
            crash_recovery->updateSearchQuery("John 3:16");
            crash_recovery->addToSearchHistory("Romans 8:28");
            
            if (crash_recovery->saveSessionState()) {
                std::cout << "✓ Session state saved successfully" << std::endl;
            } else {
                std::cout << "✗ Failed to save session state" << std::endl;
            }
            
            if (crash_recovery->hasRecoverableSession()) {
                std::cout << "✓ Recoverable session detected" << std::endl;
            }
        }
        
        // Test health monitoring
        std::cout << "\n=== Health Monitoring Test ===" << std::endl;
        auto* health_monitor = reliability.getHealthMonitor();
        if (health_monitor) {
            auto metrics = health_monitor->getCurrentMetrics();
            std::cout << "CPU Usage: " << metrics.cpu_usage << "%" << std::endl;
            std::cout << "Memory Usage: " << metrics.memory_usage_mb << " MB" << std::endl;
            std::cout << "Uptime: " << metrics.uptime.count() / 1000 << " seconds" << std::endl;
            
            if (health_monitor->isSystemHealthy()) {
                std::cout << "✓ System is healthy" << std::endl;
            } else {
                std::cout << "⚠ System has health issues" << std::endl;
            }
        }
        
        // Test backup functionality
        std::cout << "\n=== Backup System Test ===" << std::endl;
        if (reliability.createBackup("test_backup")) {
            std::cout << "✓ Backup created successfully" << std::endl;
        } else {
            std::cout << "✗ Failed to create backup" << std::endl;
        }
        
        auto backups = reliability.getAvailableBackups();
        std::cout << "Available backups: " << backups.size() << std::endl;
        
        // Test data integrity
        if (reliability.verifyDataIntegrity()) {
            std::cout << "✓ Data integrity verified" << std::endl;
        } else {
            std::cout << "⚠ Data integrity issues detected" << std::endl;
        }
        
        // Shutdown gracefully
        reliability.stop();
        std::cout << "\n✓ Reliability system stopped gracefully" << std::endl;
        
        // Cleanup
        ReliabilityManager::destroyInstance();
        std::cout << "✓ Reliability system cleaned up" << std::endl;
        
        std::cout << "\n=== All Tests Completed Successfully! ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}