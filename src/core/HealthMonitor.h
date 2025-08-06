#ifndef HEALTH_MONITOR_H
#define HEALTH_MONITOR_H

#include <string>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include <map>
#include <vector>
#include <functional>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// Forward declaration for system component enum
enum class SystemComponent;

struct ComponentHealth {
    bool is_healthy = true;
    std::string status_message;
    std::chrono::system_clock::time_point last_check;
    std::chrono::system_clock::time_point last_issue;
    int consecutive_failures = 0;
    int total_failures = 0;
    double response_time_ms = 0.0;
    json metrics;
};

struct PerformanceMetrics {
    double cpu_usage = 0.0;
    double memory_usage_mb = 0.0;
    double memory_usage_percent = 0.0;
    double disk_usage_percent = 0.0;
    int active_threads = 0;
    int open_file_handles = 0;
    double network_latency_ms = 0.0;
    std::chrono::milliseconds uptime{0};
};

struct HealthAlert {
    std::string id;
    SystemComponent component;
    std::string message;
    std::string severity; // "info", "warning", "error", "critical"
    std::chrono::system_clock::time_point timestamp;
    bool acknowledged = false;
    json additional_data;
};

class HealthMonitor {
private:
    std::atomic<bool> is_initialized{false};
    std::atomic<bool> is_monitoring{false};
    std::thread monitoring_thread;
    std::mutex health_mutex;
    std::mutex alerts_mutex;
    
    // Component health tracking
    std::map<SystemComponent, ComponentHealth> component_health;
    
    // Performance monitoring
    PerformanceMetrics current_metrics;
    std::vector<PerformanceMetrics> metrics_history;
    int max_metrics_history = 100;
    
    // Alerts and notifications
    std::vector<HealthAlert> active_alerts;
    std::vector<HealthAlert> alert_history;
    int max_alert_history = 500;
    
    // Monitoring configuration
    std::chrono::seconds monitoring_interval{5};
    std::chrono::seconds component_timeout{30};
    int max_consecutive_failures = 3;
    
    // Thresholds
    double cpu_warning_threshold = 80.0;
    double cpu_critical_threshold = 95.0;
    double memory_warning_threshold = 80.0;
    double memory_critical_threshold = 95.0;
    double disk_warning_threshold = 85.0;
    double disk_critical_threshold = 95.0;
    double response_time_warning_threshold = 1000.0; // ms
    double response_time_critical_threshold = 5000.0; // ms
    
    // Callbacks for notifications
    std::function<void(const HealthAlert&)> alert_callback;
    std::function<void(SystemComponent, bool)> component_status_callback;
    std::function<void(const PerformanceMetrics&)> performance_callback;
    
    // Component test functions
    std::map<SystemComponent, std::function<bool()>> component_tests;
    
    // Internal methods
    void monitoringLoop();
    bool checkComponentHealth(SystemComponent component);
    void updatePerformanceMetrics();
    void checkThresholds();
    void createAlert(SystemComponent component, const std::string& message, 
                    const std::string& severity, const json& data = json::object());
    void cleanupOldData();
    std::string generateAlertId();
    
    // System metrics collection
    double getCurrentCPUUsage();
    double getCurrentMemoryUsage();
    double getCurrentMemoryUsagePercent();
    double getCurrentDiskUsage();
    int getCurrentThreadCount();
    int getCurrentFileHandleCount();
    double measureNetworkLatency();
    
public:
    HealthMonitor();
    ~HealthMonitor();
    
    // Initialization and lifecycle
    bool initialize();
    void shutdown();
    bool startMonitoring();
    void stopMonitoring();
    
    // Component registration and testing
    void registerComponentTest(SystemComponent component, std::function<bool()> test_function);
    void unregisterComponentTest(SystemComponent component);
    void registerDefaultTests();
    bool performHealthCheck();
    bool isComponentHealthy(SystemComponent component);
    
    // Component issue reporting
    void reportComponentIssue(SystemComponent component, const std::string& issue);
    void reportComponentRecovery(SystemComponent component);
    void updateComponentMetrics(SystemComponent component, const json& metrics);
    
    // Performance monitoring
    PerformanceMetrics getCurrentMetrics();
    std::vector<PerformanceMetrics> getMetricsHistory(int count = 10);
    bool isPerformanceWithinThresholds();
    
    // Health status
    ComponentHealth getComponentHealth(SystemComponent component);
    std::map<SystemComponent, ComponentHealth> getAllComponentHealth();
    double getOverallHealthScore();
    bool isSystemHealthy();
    
    // Alerts and notifications
    std::vector<HealthAlert> getActiveAlerts();
    std::vector<HealthAlert> getAlertHistory(int count = 50);
    void acknowledgeAlert(const std::string& alert_id);
    void clearAlert(const std::string& alert_id);
    void clearAllAlerts();
    
    // Configuration
    void setMonitoringInterval(std::chrono::seconds interval);
    void setComponentTimeout(std::chrono::seconds timeout);
    void setMaxConsecutiveFailures(int max_failures);
    void setCPUThresholds(double warning, double critical);
    void setMemoryThresholds(double warning, double critical);
    void setDiskThresholds(double warning, double critical);
    void setResponseTimeThresholds(double warning, double critical);
    
    // Callbacks
    void setAlertCallback(std::function<void(const HealthAlert&)> callback);
    void setComponentStatusCallback(std::function<void(SystemComponent, bool)> callback);
    void setPerformanceCallback(std::function<void(const PerformanceMetrics&)> callback);
    
    // Diagnostics and reporting
    std::string generateReport();
    json exportHealthData();
    bool selfTest();
    void optimizePerformance();
    
    // Maintenance
    void resetComponentStats(SystemComponent component);
    void resetAllStats();
    bool cleanupOldFiles();
    
    // Predictive analysis
    bool detectPerformanceRegression();
    std::vector<std::string> predictPotentialIssues();
    double estimateTimeToThreshold(const std::string& metric);
    
    // Resource monitoring
    struct ResourceUsage {
        double peak_cpu = 0.0;
        double peak_memory = 0.0;
        double average_cpu = 0.0;
        double average_memory = 0.0;
        std::chrono::seconds monitoring_duration{0};
    };
    
    ResourceUsage getResourceUsageSummary();
    bool isResourceUsageExcessive();
    std::vector<std::string> getResourceOptimizationSuggestions();
    
    // Emergency detection
    bool detectEmergencyCondition();
    std::string getEmergencyReason();
    bool shouldActivateEmergencyMode();
    
    // Real-time monitoring
    void enableRealTimeMonitoring(bool enabled);
    bool isRealTimeMonitoringEnabled();
    void setHighFrequencyMonitoring(bool enabled); // For critical periods
};

#endif // HEALTH_MONITOR_H