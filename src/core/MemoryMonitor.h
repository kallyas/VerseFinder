#ifndef MEMORYMONITOR_H
#define MEMORYMONITOR_H

#include <chrono>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

struct MemorySnapshot {
    std::chrono::steady_clock::time_point timestamp;
    size_t resident_memory_mb;      // RSS in MB
    size_t virtual_memory_mb;       // Virtual memory in MB
    size_t peak_memory_mb;          // Peak memory usage in MB
    size_t heap_memory_mb;          // Heap memory in MB (if available)
    double cpu_usage_percent;       // CPU usage percentage
    
    MemorySnapshot() : timestamp(std::chrono::steady_clock::now()),
                      resident_memory_mb(0), virtual_memory_mb(0), 
                      peak_memory_mb(0), heap_memory_mb(0), cpu_usage_percent(0.0) {}
};

class MemoryMonitor {
private:
    std::atomic<bool> monitoring{false};
    std::thread monitor_thread;
    std::vector<MemorySnapshot> snapshots;
    mutable std::mutex snapshots_mutex;
    std::condition_variable stop_condition;
    
    // Configuration
    std::chrono::milliseconds sample_interval{1000}; // 1 second default
    size_t max_snapshots = 1000; // Keep last 1000 snapshots
    
    // Monitoring state
    size_t peak_memory_ever = 0;
    std::atomic<size_t> current_memory{0};
    
    // Platform-specific memory reading functions
    MemorySnapshot getCurrentMemoryInfo() const;
    
#ifdef _WIN32
    MemorySnapshot getWindowsMemoryInfo() const;
#elif __APPLE__
    MemorySnapshot getMacOSMemoryInfo() const;
#elif __linux__
    MemorySnapshot getLinuxMemoryInfo() const;
#endif
    
    void monitoringLoop();
    void trimSnapshots();

public:
    MemoryMonitor() = default;
    ~MemoryMonitor();
    
    // Non-copyable
    MemoryMonitor(const MemoryMonitor&) = delete;
    MemoryMonitor& operator=(const MemoryMonitor&) = delete;
    
    // Control monitoring
    void startMonitoring(std::chrono::milliseconds interval = std::chrono::milliseconds(1000));
    void stopMonitoring();
    bool isMonitoring() const { return monitoring.load(); }
    
    // Get current memory info
    MemorySnapshot getCurrentSnapshot() const;
    size_t getCurrentMemoryMB() const { return current_memory.load(); }
    size_t getPeakMemoryMB() const { return peak_memory_ever; }
    
    // Get historical data
    std::vector<MemorySnapshot> getSnapshots(size_t max_count = 100) const;
    std::vector<MemorySnapshot> getSnapshotsInRange(
        std::chrono::steady_clock::time_point start,
        std::chrono::steady_clock::time_point end) const;
    
    // Statistics
    MemorySnapshot getAverageUsage() const;
    MemorySnapshot getMaxUsage() const;
    MemorySnapshot getMinUsage() const;
    
    // Memory thresholds and alerts
    void setMemoryThreshold(size_t threshold_mb);
    bool isMemoryThresholdExceeded() const;
    void clearPeakMemory();
    
    // Configuration
    void setSampleInterval(std::chrono::milliseconds interval);
    void setMaxSnapshots(size_t max_snapshots);
    
    // Export data
    bool exportToCsv(const std::string& filename) const;
    std::string getMemoryReport() const;
    
private:
    size_t memory_threshold_mb = 200; // Default threshold from requirements
    
    // Helper functions
    static size_t bytesToMB(size_t bytes) { return bytes / (1024 * 1024); }
    static std::string formatTimestamp(const std::chrono::steady_clock::time_point& tp);
};

// Global memory monitor instance
extern MemoryMonitor g_memory_monitor;

// Convenience macros
#define MEMORY_MONITOR_START() g_memory_monitor.startMonitoring()
#define MEMORY_MONITOR_STOP() g_memory_monitor.stopMonitoring()
#define MEMORY_MONITOR_CURRENT() g_memory_monitor.getCurrentMemoryMB()
#define MEMORY_MONITOR_PEAK() g_memory_monitor.getPeakMemoryMB()

#endif // MEMORYMONITOR_H