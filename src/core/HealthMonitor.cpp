#include "HealthMonitor.h"
#include "ReliabilityManager.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <random>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <pdh.h>
#include <psapi.h>
#elif defined(__linux__)
#include <unistd.h>
#include <sys/resource.h>
#include <sys/statvfs.h>
#include <fstream>
#elif defined(__APPLE__)
#include <unistd.h>
#include <sys/resource.h>
#include <sys/statvfs.h>
#include <mach/mach.h>
#endif

HealthMonitor::HealthMonitor() {
    // Initialize component health for all system components
    for (int i = 0; i <= static_cast<int>(SystemComponent::FILE_SYSTEM); ++i) {
        SystemComponent component = static_cast<SystemComponent>(i);
        component_health[component] = ComponentHealth();
    }
}

HealthMonitor::~HealthMonitor() {
    shutdown();
}

bool HealthMonitor::initialize() {
    if (is_initialized.load()) {
        return true;
    }
    
    try {
        // Initialize performance metrics
        updatePerformanceMetrics();
        
        // Register default component tests
        registerDefaultTests();
        
        is_initialized.store(true);
        
        std::cout << "HealthMonitor initialized successfully" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize HealthMonitor: " << e.what() << std::endl;
        return false;
    }
}

void HealthMonitor::shutdown() {
    if (!is_initialized.load()) {
        return;
    }
    
    stopMonitoring();
    is_initialized.store(false);
    
    std::cout << "HealthMonitor shutdown complete" << std::endl;
}

bool HealthMonitor::startMonitoring() {
    if (!is_initialized.load() || is_monitoring.load()) {
        return false;
    }
    
    try {
        is_monitoring.store(true);
        monitoring_thread = std::thread(&HealthMonitor::monitoringLoop, this);
        
        std::cout << "Health monitoring started" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to start health monitoring: " << e.what() << std::endl;
        is_monitoring.store(false);
        return false;
    }
}

void HealthMonitor::stopMonitoring() {
    if (!is_monitoring.load()) {
        return;
    }
    
    is_monitoring.store(false);
    
    if (monitoring_thread.joinable()) {
        monitoring_thread.join();
    }
    
    std::cout << "Health monitoring stopped" << std::endl;
}

void HealthMonitor::monitoringLoop() {
    while (is_monitoring.load()) {
        try {
            // Update performance metrics
            updatePerformanceMetrics();
            
            // Check all registered components
            performHealthCheck();
            
            // Check thresholds and create alerts if necessary
            checkThresholds();
            
            // Cleanup old data periodically
            static int cleanup_counter = 0;
            if (++cleanup_counter >= 60) { // Every 5 minutes at 5-second intervals
                cleanupOldData();
                cleanup_counter = 0;
            }
            
            // Notify performance callback
            if (performance_callback) {
                performance_callback(current_metrics);
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error in health monitoring loop: " << e.what() << std::endl;
        }
        
        // Sleep for monitoring interval
        std::this_thread::sleep_for(monitoring_interval);
    }
}

void HealthMonitor::registerComponentTest(SystemComponent component, std::function<bool()> test_function) {
    component_tests[component] = test_function;
}

void HealthMonitor::unregisterComponentTest(SystemComponent component) {
    component_tests.erase(component);
}

void HealthMonitor::registerDefaultTests() {
    // Core engine test
    registerComponentTest(SystemComponent::CORE_ENGINE, []() {
        // Basic functionality test
        return true; // Assume healthy if we can execute this
    });
    
    // Memory management test
    registerComponentTest(SystemComponent::MEMORY_MANAGEMENT, []() {
        try {
            // Test memory allocation
            auto test_ptr = std::make_unique<std::vector<int>>(1000);
            return test_ptr != nullptr;
        } catch (const std::exception&) {
            return false;
        }
    });
    
    // File system test
    registerComponentTest(SystemComponent::FILE_SYSTEM, []() {
        try {
            // Test file system access
            std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
            return std::filesystem::exists(temp_dir) && std::filesystem::is_directory(temp_dir);
        } catch (const std::exception&) {
            return false;
        }
    });
    
    // Data storage test
    registerComponentTest(SystemComponent::DATA_STORAGE, []() {
        try {
            // Test that we can create a temporary file
            std::filesystem::path temp_file = std::filesystem::temp_directory_path() / "health_test.tmp";
            std::ofstream file(temp_file);
            bool success = file.is_open();
            file.close();
            if (success) {
                std::filesystem::remove(temp_file);
            }
            return success;
        } catch (const std::exception&) {
            return false;
        }
    });
}

bool HealthMonitor::performHealthCheck() {
    bool overall_healthy = true;
    
    std::lock_guard<std::mutex> lock(health_mutex);
    
    for (auto& [component, health] : component_health) {
        bool was_healthy = health.is_healthy;
        
        // Check if we have a test for this component
        auto test_it = component_tests.find(component);
        if (test_it != component_tests.end()) {
            try {
                auto start_time = std::chrono::high_resolution_clock::now();
                bool test_result = test_it->second();
                auto end_time = std::chrono::high_resolution_clock::now();
                
                health.response_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
                
                if (test_result) {
                    if (!was_healthy) {
                        // Component recovered
                        health.is_healthy = true;
                        health.status_message = "Component recovered";
                        health.consecutive_failures = 0;
                        
                        if (component_status_callback) {
                            component_status_callback(component, true);
                        }
                    } else {
                        health.status_message = "Component healthy";
                    }
                } else {
                    // Component failed test
                    health.consecutive_failures++;
                    health.total_failures++;
                    health.last_issue = std::chrono::system_clock::now();
                    
                    if (health.consecutive_failures >= max_consecutive_failures) {
                        health.is_healthy = false;
                        health.status_message = "Component failed health check";
                        
                        if (was_healthy && component_status_callback) {
                            component_status_callback(component, false);
                        }
                        
                        // Create alert for component failure
                        createAlert(component, "Component health check failed", "error");
                    }
                }
                
            } catch (const std::exception& e) {
                health.consecutive_failures++;
                health.total_failures++;
                health.last_issue = std::chrono::system_clock::now();
                health.status_message = "Health check exception: " + std::string(e.what());
                
                if (health.consecutive_failures >= max_consecutive_failures) {
                    health.is_healthy = false;
                    
                    if (was_healthy && component_status_callback) {
                        component_status_callback(component, false);
                    }
                }
            }
        }
        
        health.last_check = std::chrono::system_clock::now();
        
        if (!health.is_healthy) {
            overall_healthy = false;
        }
    }
    
    return overall_healthy;
}

bool HealthMonitor::isComponentHealthy(SystemComponent component) {
    std::lock_guard<std::mutex> lock(health_mutex);
    auto it = component_health.find(component);
    return it != component_health.end() ? it->second.is_healthy : true;
}

void HealthMonitor::reportComponentIssue(SystemComponent component, const std::string& issue) {
    std::lock_guard<std::mutex> lock(health_mutex);
    
    auto& health = component_health[component];
    health.consecutive_failures++;
    health.total_failures++;
    health.last_issue = std::chrono::system_clock::now();
    health.status_message = issue;
    
    if (health.consecutive_failures >= max_consecutive_failures) {
        bool was_healthy = health.is_healthy;
        health.is_healthy = false;
        
        if (was_healthy && component_status_callback) {
            component_status_callback(component, false);
        }
        
        createAlert(component, issue, "error");
    }
}

void HealthMonitor::reportComponentRecovery(SystemComponent component) {
    std::lock_guard<std::mutex> lock(health_mutex);
    
    auto& health = component_health[component];
    bool was_unhealthy = !health.is_healthy;
    
    health.is_healthy = true;
    health.consecutive_failures = 0;
    health.status_message = "Component recovered";
    health.last_check = std::chrono::system_clock::now();
    
    if (was_unhealthy && component_status_callback) {
        component_status_callback(component, true);
    }
    
    createAlert(component, "Component recovered", "info");
}

void HealthMonitor::updateComponentMetrics(SystemComponent component, const json& metrics) {
    std::lock_guard<std::mutex> lock(health_mutex);
    component_health[component].metrics = metrics;
}

void HealthMonitor::updatePerformanceMetrics() {
    PerformanceMetrics new_metrics;
    
    // Get system metrics
    new_metrics.cpu_usage = getCurrentCPUUsage();
    new_metrics.memory_usage_mb = getCurrentMemoryUsage();
    new_metrics.memory_usage_percent = getCurrentMemoryUsagePercent();
    new_metrics.disk_usage_percent = getCurrentDiskUsage();
    new_metrics.active_threads = getCurrentThreadCount();
    new_metrics.open_file_handles = getCurrentFileHandleCount();
    new_metrics.network_latency_ms = measureNetworkLatency();
    
    // Calculate uptime
    static auto start_time = std::chrono::system_clock::now();
    new_metrics.uptime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - start_time);
    
    // Update current metrics
    current_metrics = new_metrics;
    
    // Add to history
    metrics_history.push_back(new_metrics);
    if (metrics_history.size() > static_cast<size_t>(max_metrics_history)) {
        metrics_history.erase(metrics_history.begin());
    }
}

void HealthMonitor::checkThresholds() {
    // Check CPU usage
    if (current_metrics.cpu_usage > cpu_critical_threshold) {
        createAlert(SystemComponent::CORE_ENGINE, 
                   "CPU usage critical: " + std::to_string(current_metrics.cpu_usage) + "%", 
                   "critical");
    } else if (current_metrics.cpu_usage > cpu_warning_threshold) {
        createAlert(SystemComponent::CORE_ENGINE, 
                   "CPU usage high: " + std::to_string(current_metrics.cpu_usage) + "%", 
                   "warning");
    }
    
    // Check memory usage
    if (current_metrics.memory_usage_percent > memory_critical_threshold) {
        createAlert(SystemComponent::MEMORY_MANAGEMENT, 
                   "Memory usage critical: " + std::to_string(current_metrics.memory_usage_percent) + "%", 
                   "critical");
    } else if (current_metrics.memory_usage_percent > memory_warning_threshold) {
        createAlert(SystemComponent::MEMORY_MANAGEMENT, 
                   "Memory usage high: " + std::to_string(current_metrics.memory_usage_percent) + "%", 
                   "warning");
    }
    
    // Check disk usage
    if (current_metrics.disk_usage_percent > disk_critical_threshold) {
        createAlert(SystemComponent::FILE_SYSTEM, 
                   "Disk usage critical: " + std::to_string(current_metrics.disk_usage_percent) + "%", 
                   "critical");
    } else if (current_metrics.disk_usage_percent > disk_warning_threshold) {
        createAlert(SystemComponent::FILE_SYSTEM, 
                   "Disk usage high: " + std::to_string(current_metrics.disk_usage_percent) + "%", 
                   "warning");
    }
}

void HealthMonitor::createAlert(SystemComponent component, const std::string& message, 
                               const std::string& severity, const json& data) {
    std::lock_guard<std::mutex> lock(alerts_mutex);
    
    HealthAlert alert;
    alert.id = generateAlertId();
    alert.component = component;
    alert.message = message;
    alert.severity = severity;
    alert.timestamp = std::chrono::system_clock::now();
    alert.additional_data = data;
    
    active_alerts.push_back(alert);
    alert_history.push_back(alert);
    
    // Maintain history size
    if (alert_history.size() > static_cast<size_t>(max_alert_history)) {
        alert_history.erase(alert_history.begin());
    }
    
    // Notify callback
    if (alert_callback) {
        alert_callback(alert);
    }
    
    std::cout << "Health Alert [" << severity << "]: " << message << std::endl;
}

void HealthMonitor::cleanupOldData() {
    // Clean up old alerts (remove acknowledged ones older than 1 hour)
    std::lock_guard<std::mutex> lock(alerts_mutex);
    
    auto now = std::chrono::system_clock::now();
    auto one_hour_ago = now - std::chrono::hours(1);
    
    active_alerts.erase(
        std::remove_if(active_alerts.begin(), active_alerts.end(),
                      [one_hour_ago](const HealthAlert& alert) {
                          return alert.acknowledged && alert.timestamp < one_hour_ago;
                      }),
        active_alerts.end());
}

std::string HealthMonitor::generateAlertId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(100000, 999999);
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "ALERT_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    ss << "_" << dis(gen);
    
    return ss.str();
}

// System metrics collection methods (platform-specific implementations)

double HealthMonitor::getCurrentCPUUsage() {
#ifdef _WIN32
    // Windows implementation
    static ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
    static int numProcessors = 0;
    static bool first_call = true;
    
    if (first_call) {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        numProcessors = sysInfo.dwNumberOfProcessors;
        
        FILETIME ftime, fsys, fuser;
        GetSystemTimeAsFileTime(&ftime);
        memcpy(&lastCPU, &ftime, sizeof(FILETIME));
        
        GetProcessTimes(GetCurrentProcess(), &ftime, &ftime, &fsys, &fuser);
        memcpy(&lastSysCPU, &fsys, sizeof(FILETIME));
        memcpy(&lastUserCPU, &fuser, sizeof(FILETIME));
        
        first_call = false;
        return 0.0;
    }
    
    FILETIME ftime, fsys, fuser;
    ULARGE_INTEGER now, sys, user;
    
    GetSystemTimeAsFileTime(&ftime);
    memcpy(&now, &ftime, sizeof(FILETIME));
    
    GetProcessTimes(GetCurrentProcess(), &ftime, &ftime, &fsys, &fuser);
    memcpy(&sys, &fsys, sizeof(FILETIME));
    memcpy(&user, &fuser, sizeof(FILETIME));
    
    double percent = (sys.QuadPart - lastSysCPU.QuadPart) + (user.QuadPart - lastUserCPU.QuadPart);
    percent /= (now.QuadPart - lastCPU.QuadPart);
    percent /= numProcessors;
    lastCPU = now;
    lastUserCPU = user;
    lastSysCPU = sys;
    
    return percent * 100.0;
    
#elif defined(__linux__)
    // Linux implementation
    static long long lastTotalUser = 0, lastTotalSys = 0, lastTotalIdle = 0;
    static bool first_call = true;
    
    std::ifstream file("/proc/stat");
    if (!file.is_open()) return 0.0;
    
    std::string line;
    std::getline(file, line);
    file.close();
    
    long long user, nice, system, idle;
    sscanf(line.c_str(), "cpu %lld %lld %lld %lld", &user, &nice, &system, &idle);
    
    if (first_call) {
        lastTotalUser = user + nice;
        lastTotalSys = system;
        lastTotalIdle = idle;
        first_call = false;
        return 0.0;
    }
    
    long long totalUser = user + nice;
    long long totalSys = system;
    long long totalIdle = idle;
    
    long long deltaUser = totalUser - lastTotalUser;
    long long deltaSys = totalSys - lastTotalSys;
    long long deltaIdle = totalIdle - lastTotalIdle;
    long long deltaTotal = deltaUser + deltaSys + deltaIdle;
    
    double cpuUsage = 0.0;
    if (deltaTotal > 0) {
        cpuUsage = 100.0 * (deltaUser + deltaSys) / deltaTotal;
    }
    
    lastTotalUser = totalUser;
    lastTotalSys = totalSys;
    lastTotalIdle = totalIdle;
    
    return cpuUsage;
    
#elif defined(__APPLE__)
    // macOS implementation (simplified)
    // This is a basic implementation - a more detailed one would use mach calls
    return 0.0; // Placeholder
    
#else
    return 0.0; // Unsupported platform
#endif
}

double HealthMonitor::getCurrentMemoryUsage() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return static_cast<double>(pmc.WorkingSetSize) / (1024.0 * 1024.0); // MB
    }
    return 0.0;
    
#elif defined(__linux__)
    std::ifstream file("/proc/self/status");
    if (!file.is_open()) return 0.0;
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("VmRSS:") == 0) {
            long long rss_kb;
            sscanf(line.c_str(), "VmRSS: %lld kB", &rss_kb);
            return static_cast<double>(rss_kb) / 1024.0; // MB
        }
    }
    return 0.0;
    
#elif defined(__APPLE__)
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return static_cast<double>(usage.ru_maxrss) / (1024.0 * 1024.0); // MB
    }
    return 0.0;
    
#else
    return 0.0; // Unsupported platform
#endif
}

double HealthMonitor::getCurrentMemoryUsagePercent() {
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        return static_cast<double>(memInfo.dwMemoryLoad);
    }
    return 0.0;
    
#elif defined(__linux__)
    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) return 0.0;
    
    long long total_kb = 0, available_kb = 0;
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.find("MemTotal:") == 0) {
            sscanf(line.c_str(), "MemTotal: %lld kB", &total_kb);
        } else if (line.find("MemAvailable:") == 0) {
            sscanf(line.c_str(), "MemAvailable: %lld kB", &available_kb);
            break;
        }
    }
    
    if (total_kb > 0) {
        return 100.0 * (total_kb - available_kb) / total_kb;
    }
    return 0.0;
    
#else
    return 0.0; // Simplified for other platforms
#endif
}

double HealthMonitor::getCurrentDiskUsage() {
    try {
        std::filesystem::path current_path = std::filesystem::current_path();
        auto space_info = std::filesystem::space(current_path);
        
        if (space_info.capacity > 0) {
            auto used = space_info.capacity - space_info.available;
            return 100.0 * used / space_info.capacity;
        }
    } catch (const std::exception&) {
        // Fall back to platform-specific methods if filesystem::space fails
    }
    
    return 0.0;
}

int HealthMonitor::getCurrentThreadCount() {
#ifdef _WIN32
    // Windows thread count would require more complex implementation
    return 1; // Placeholder
    
#elif defined(__linux__)
    std::ifstream file("/proc/self/status");
    if (!file.is_open()) return 1;
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("Threads:") == 0) {
            int threads;
            sscanf(line.c_str(), "Threads: %d", &threads);
            return threads;
        }
    }
    return 1;
    
#else
    return 1; // Default
#endif
}

int HealthMonitor::getCurrentFileHandleCount() {
#ifdef __linux__
    try {
        std::filesystem::path fd_dir = "/proc/self/fd";
        int count = 0;
        for (const auto& entry : std::filesystem::directory_iterator(fd_dir)) {
            (void)entry; // Suppress unused warning
            ++count;
        }
        return count;
    } catch (const std::exception&) {
        return 0;
    }
#else
    return 0; // Not implemented for other platforms
#endif
}

double HealthMonitor::measureNetworkLatency() {
    // This is a simplified implementation
    // In practice, you might ping a known server or measure internal network operations
    return 0.0; // Placeholder
}

// Public interface methods

PerformanceMetrics HealthMonitor::getCurrentMetrics() {
    return current_metrics;
}

std::vector<PerformanceMetrics> HealthMonitor::getMetricsHistory(int count) {
    if (count <= 0 || metrics_history.empty()) {
        return {};
    }
    
    int start = std::max(0, static_cast<int>(metrics_history.size()) - count);
    return std::vector<PerformanceMetrics>(metrics_history.begin() + start, metrics_history.end());
}

bool HealthMonitor::isPerformanceWithinThresholds() {
    return current_metrics.cpu_usage < cpu_warning_threshold &&
           current_metrics.memory_usage_percent < memory_warning_threshold &&
           current_metrics.disk_usage_percent < disk_warning_threshold;
}

ComponentHealth HealthMonitor::getComponentHealth(SystemComponent component) {
    std::lock_guard<std::mutex> lock(health_mutex);
    auto it = component_health.find(component);
    return it != component_health.end() ? it->second : ComponentHealth();
}

std::map<SystemComponent, ComponentHealth> HealthMonitor::getAllComponentHealth() {
    std::lock_guard<std::mutex> lock(health_mutex);
    return component_health;
}

double HealthMonitor::getOverallHealthScore() {
    std::lock_guard<std::mutex> lock(health_mutex);
    
    if (component_health.empty()) {
        return 1.0;
    }
    
    int healthy_components = 0;
    for (const auto& [component, health] : component_health) {
        if (health.is_healthy) {
            healthy_components++;
        }
    }
    
    double component_score = static_cast<double>(healthy_components) / component_health.size();
    
    // Factor in performance metrics
    double perf_score = 1.0;
    if (current_metrics.cpu_usage > cpu_critical_threshold) perf_score *= 0.5;
    else if (current_metrics.cpu_usage > cpu_warning_threshold) perf_score *= 0.8;
    
    if (current_metrics.memory_usage_percent > memory_critical_threshold) perf_score *= 0.5;
    else if (current_metrics.memory_usage_percent > memory_warning_threshold) perf_score *= 0.8;
    
    return component_score * perf_score;
}

bool HealthMonitor::isSystemHealthy() {
    return getOverallHealthScore() > 0.8;
}

std::vector<HealthAlert> HealthMonitor::getActiveAlerts() {
    std::lock_guard<std::mutex> lock(alerts_mutex);
    return active_alerts;
}

std::vector<HealthAlert> HealthMonitor::getAlertHistory(int count) {
    std::lock_guard<std::mutex> lock(alerts_mutex);
    
    if (count <= 0 || alert_history.empty()) {
        return {};
    }
    
    int start = std::max(0, static_cast<int>(alert_history.size()) - count);
    return std::vector<HealthAlert>(alert_history.begin() + start, alert_history.end());
}

void HealthMonitor::acknowledgeAlert(const std::string& alert_id) {
    std::lock_guard<std::mutex> lock(alerts_mutex);
    
    for (auto& alert : active_alerts) {
        if (alert.id == alert_id) {
            alert.acknowledged = true;
            break;
        }
    }
}

void HealthMonitor::clearAlert(const std::string& alert_id) {
    std::lock_guard<std::mutex> lock(alerts_mutex);
    
    active_alerts.erase(
        std::remove_if(active_alerts.begin(), active_alerts.end(),
                      [&alert_id](const HealthAlert& alert) {
                          return alert.id == alert_id;
                      }),
        active_alerts.end());
}

void HealthMonitor::clearAllAlerts() {
    std::lock_guard<std::mutex> lock(alerts_mutex);
    active_alerts.clear();
}

// Configuration methods
void HealthMonitor::setMonitoringInterval(std::chrono::seconds interval) {
    monitoring_interval = interval;
}

void HealthMonitor::setComponentTimeout(std::chrono::seconds timeout) {
    component_timeout = timeout;
}

void HealthMonitor::setMaxConsecutiveFailures(int max_failures) {
    max_consecutive_failures = max_failures;
}

void HealthMonitor::setCPUThresholds(double warning, double critical) {
    cpu_warning_threshold = warning;
    cpu_critical_threshold = critical;
}

void HealthMonitor::setMemoryThresholds(double warning, double critical) {
    memory_warning_threshold = warning;
    memory_critical_threshold = critical;
}

void HealthMonitor::setDiskThresholds(double warning, double critical) {
    disk_warning_threshold = warning;
    disk_critical_threshold = critical;
}

void HealthMonitor::setResponseTimeThresholds(double warning, double critical) {
    response_time_warning_threshold = warning;
    response_time_critical_threshold = critical;
}

// Callback setters
void HealthMonitor::setAlertCallback(std::function<void(const HealthAlert&)> callback) {
    alert_callback = callback;
}

void HealthMonitor::setComponentStatusCallback(std::function<void(SystemComponent, bool)> callback) {
    component_status_callback = callback;
}

void HealthMonitor::setPerformanceCallback(std::function<void(const PerformanceMetrics&)> callback) {
    performance_callback = callback;
}

std::string HealthMonitor::generateReport() {
    std::stringstream report;
    
    report << "\n=== Health Monitor Report ===\n";
    
    // Overall health
    report << "Overall Health Score: " << std::fixed << std::setprecision(2) 
           << (getOverallHealthScore() * 100) << "%\n";
    report << "System Status: " << (isSystemHealthy() ? "HEALTHY" : "DEGRADED") << "\n";
    
    // Performance metrics
    report << "\nPerformance Metrics:\n";
    report << "  CPU Usage: " << std::fixed << std::setprecision(1) << current_metrics.cpu_usage << "%\n";
    report << "  Memory Usage: " << std::fixed << std::setprecision(1) 
           << current_metrics.memory_usage_mb << " MB (" 
           << current_metrics.memory_usage_percent << "%)\n";
    report << "  Disk Usage: " << std::fixed << std::setprecision(1) << current_metrics.disk_usage_percent << "%\n";
    report << "  Active Threads: " << current_metrics.active_threads << "\n";
    report << "  Uptime: " << current_metrics.uptime.count() / 1000 << " seconds\n";
    
    // Component health
    report << "\nComponent Health:\n";
    {
        std::lock_guard<std::mutex> lock(health_mutex);
        for (const auto& [component, health] : component_health) {
            report << "  Component " << static_cast<int>(component) << ": " 
                   << (health.is_healthy ? "HEALTHY" : "UNHEALTHY") 
                   << " (" << health.status_message << ")\n";
        }
    }
    
    // Active alerts
    auto alerts = getActiveAlerts();
    report << "\nActive Alerts: " << alerts.size() << "\n";
    for (const auto& alert : alerts) {
        auto time_t = std::chrono::system_clock::to_time_t(alert.timestamp);
        report << "  [" << alert.severity << "] " << alert.message 
               << " (" << std::put_time(std::localtime(&time_t), "%H:%M:%S") << ")\n";
    }
    
    return report.str();
}

json HealthMonitor::exportHealthData() {
    json data;
    
    // Current metrics
    data["current_metrics"] = {
        {"cpu_usage", current_metrics.cpu_usage},
        {"memory_usage_mb", current_metrics.memory_usage_mb},
        {"memory_usage_percent", current_metrics.memory_usage_percent},
        {"disk_usage_percent", current_metrics.disk_usage_percent},
        {"active_threads", current_metrics.active_threads},
        {"uptime_ms", current_metrics.uptime.count()}
    };
    
    // Component health
    json components = json::array();
    {
        std::lock_guard<std::mutex> lock(health_mutex);
        for (const auto& [component, health] : component_health) {
            json comp_data = {
                {"component", static_cast<int>(component)},
                {"is_healthy", health.is_healthy},
                {"status_message", health.status_message},
                {"consecutive_failures", health.consecutive_failures},
                {"total_failures", health.total_failures},
                {"response_time_ms", health.response_time_ms}
            };
            components.push_back(comp_data);
        }
    }
    data["components"] = components;
    
    // Active alerts
    json alerts = json::array();
    {
        std::lock_guard<std::mutex> lock(alerts_mutex);
        for (const auto& alert : active_alerts) {
            json alert_data = {
                {"id", alert.id},
                {"component", static_cast<int>(alert.component)},
                {"message", alert.message},
                {"severity", alert.severity},
                {"acknowledged", alert.acknowledged}
            };
            alerts.push_back(alert_data);
        }
    }
    data["active_alerts"] = alerts;
    
    return data;
}

bool HealthMonitor::selfTest() {
    try {
        // Test performance metrics collection
        updatePerformanceMetrics();
        
        // Test component health check
        performHealthCheck();
        
        // Test alert creation
        createAlert(SystemComponent::CORE_ENGINE, "Self-test alert", "info");
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "HealthMonitor self-test failed: " << e.what() << std::endl;
        return false;
    }
}

void HealthMonitor::optimizePerformance() {
    // Clean up old data
    cleanupOldData();
    
    // Reduce metrics history if memory usage is high
    if (current_metrics.memory_usage_percent > memory_warning_threshold) {
        max_metrics_history = std::max(10, max_metrics_history / 2);
        if (metrics_history.size() > static_cast<size_t>(max_metrics_history)) {
            metrics_history.erase(metrics_history.begin(), 
                                 metrics_history.end() - max_metrics_history);
        }
    }
}

bool HealthMonitor::detectEmergencyCondition() {
    return current_metrics.cpu_usage > cpu_critical_threshold ||
           current_metrics.memory_usage_percent > memory_critical_threshold ||
           current_metrics.disk_usage_percent > disk_critical_threshold ||
           getOverallHealthScore() < 0.3;
}

std::string HealthMonitor::getEmergencyReason() {
    std::vector<std::string> reasons;
    
    if (current_metrics.cpu_usage > cpu_critical_threshold) {
        reasons.push_back("Critical CPU usage: " + std::to_string(current_metrics.cpu_usage) + "%");
    }
    
    if (current_metrics.memory_usage_percent > memory_critical_threshold) {
        reasons.push_back("Critical memory usage: " + std::to_string(current_metrics.memory_usage_percent) + "%");
    }
    
    if (current_metrics.disk_usage_percent > disk_critical_threshold) {
        reasons.push_back("Critical disk usage: " + std::to_string(current_metrics.disk_usage_percent) + "%");
    }
    
    if (getOverallHealthScore() < 0.3) {
        reasons.push_back("Overall system health critically low");
    }
    
    if (reasons.empty()) {
        return "No emergency condition detected";
    }
    
    std::string result = "Emergency condition: ";
    for (size_t i = 0; i < reasons.size(); ++i) {
        if (i > 0) result += "; ";
        result += reasons[i];
    }
    
    return result;
}

bool HealthMonitor::shouldActivateEmergencyMode() {
    return detectEmergencyCondition();
}

void HealthMonitor::resetComponentStats(SystemComponent component) {
    std::lock_guard<std::mutex> lock(health_mutex);
    auto& health = component_health[component];
    health.consecutive_failures = 0;
    health.total_failures = 0;
}

void HealthMonitor::resetAllStats() {
    std::lock_guard<std::mutex> lock(health_mutex);
    for (auto& [component, health] : component_health) {
        health.consecutive_failures = 0;
        health.total_failures = 0;
    }
}

bool HealthMonitor::cleanupOldFiles() {
    // This could clean up temporary files, logs, etc.
    return true; // Placeholder implementation
}