#include "MemoryMonitor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#elif __APPLE__
#include <mach/mach.h>
#include <mach/task.h>
#include <mach/mach_init.h>
#include <sys/resource.h>
#elif __linux__
#include <unistd.h>
#include <fstream>
#include <string>
#include <sys/resource.h>
#endif

// Global memory monitor instance
MemoryMonitor g_memory_monitor;

MemoryMonitor::~MemoryMonitor() {
    stopMonitoring();
}

void MemoryMonitor::startMonitoring(std::chrono::milliseconds interval) {
    if (monitoring.load()) {
        return; // Already monitoring
    }
    
    sample_interval = interval;
    monitoring.store(true);
    
    monitor_thread = std::thread(&MemoryMonitor::monitoringLoop, this);
}

void MemoryMonitor::stopMonitoring() {
    if (!monitoring.load()) {
        return; // Not monitoring
    }
    
    monitoring.store(false);
    stop_condition.notify_all();
    
    if (monitor_thread.joinable()) {
        monitor_thread.join();
    }
}

void MemoryMonitor::monitoringLoop() {
    std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);
    
    while (monitoring.load()) {
        MemorySnapshot snapshot = getCurrentMemoryInfo();
        
        {
            std::lock_guard<std::mutex> snap_lock(snapshots_mutex);
            snapshots.push_back(snapshot);
            trimSnapshots();
        }
        
        // Update current memory and peak
        current_memory.store(snapshot.resident_memory_mb);
        if (snapshot.resident_memory_mb > peak_memory_ever) {
            peak_memory_ever = snapshot.resident_memory_mb;
        }
        
        // Wait for the next sample interval or stop signal
        if (stop_condition.wait_for(lock, sample_interval, [this] { return !monitoring.load(); })) {
            break; // Stop signal received
        }
    }
}

void MemoryMonitor::trimSnapshots() {
    if (snapshots.size() > max_snapshots) {
        // Remove oldest snapshots
        size_t to_remove = snapshots.size() - max_snapshots;
        snapshots.erase(snapshots.begin(), snapshots.begin() + to_remove);
    }
}

MemorySnapshot MemoryMonitor::getCurrentSnapshot() const {
    return getCurrentMemoryInfo();
}

MemorySnapshot MemoryMonitor::getCurrentMemoryInfo() const {
#ifdef _WIN32
    return getWindowsMemoryInfo();
#elif __APPLE__
    return getMacOSMemoryInfo();
#elif __linux__
    return getLinuxMemoryInfo();
#else
    // Fallback for unknown platforms
    MemorySnapshot snapshot;
    return snapshot;
#endif
}

#ifdef _WIN32
MemorySnapshot MemoryMonitor::getWindowsMemoryInfo() const {
    MemorySnapshot snapshot;
    
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        snapshot.resident_memory_mb = bytesToMB(pmc.WorkingSetSize);
        snapshot.virtual_memory_mb = bytesToMB(pmc.PagefileUsage);
        snapshot.peak_memory_mb = bytesToMB(pmc.PeakWorkingSetSize);
    }
    
    // Get CPU usage (simplified)
    FILETIME idle_time, kernel_time, user_time;
    if (GetSystemTimes(&idle_time, &kernel_time, &user_time)) {
        // This is a simplified CPU calculation
        // In a real implementation, you'd want to track changes over time
        snapshot.cpu_usage_percent = 0.0; // Placeholder
    }
    
    return snapshot;
}
#endif

#ifdef __APPLE__
MemorySnapshot MemoryMonitor::getMacOSMemoryInfo() const {
    MemorySnapshot snapshot;
    
    mach_task_basic_info_data_t info;
    mach_msg_type_number_t info_count = MACH_TASK_BASIC_INFO_COUNT;
    
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, 
                  (task_info_t)&info, &info_count) == KERN_SUCCESS) {
        snapshot.resident_memory_mb = bytesToMB(info.resident_size);
        snapshot.virtual_memory_mb = bytesToMB(info.virtual_size);
    }
    
    // Get peak memory from resource usage
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        snapshot.peak_memory_mb = bytesToMB(usage.ru_maxrss);
    }
    
    return snapshot;
}
#endif

#ifdef __linux__
MemorySnapshot MemoryMonitor::getLinuxMemoryInfo() const {
    MemorySnapshot snapshot;
    
    // Read from /proc/self/status
    std::ifstream status_file("/proc/self/status");
    std::string line;
    
    while (std::getline(status_file, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            std::istringstream iss(line.substr(6));
            size_t kb;
            iss >> kb;
            snapshot.resident_memory_mb = kb / 1024;
        } else if (line.substr(0, 7) == "VmSize:") {
            std::istringstream iss(line.substr(7));
            size_t kb;
            iss >> kb;
            snapshot.virtual_memory_mb = kb / 1024;
        } else if (line.substr(0, 6) == "VmPeak:") {
            std::istringstream iss(line.substr(6));
            size_t kb;
            iss >> kb;
            snapshot.peak_memory_mb = kb / 1024;
        }
    }
    
    // Get CPU usage from /proc/self/stat (simplified)
    std::ifstream stat_file("/proc/self/stat");
    if (stat_file.is_open()) {
        std::string stat_line;
        std::getline(stat_file, stat_line);
        // Parse CPU times from stat file (simplified implementation)
        snapshot.cpu_usage_percent = 0.0; // Placeholder
    }
    
    return snapshot;
}
#endif

std::vector<MemorySnapshot> MemoryMonitor::getSnapshots(size_t max_count) const {
    std::lock_guard<std::mutex> lock(snapshots_mutex);
    
    if (snapshots.size() <= max_count) {
        return snapshots;
    }
    
    // Return the most recent snapshots
    return std::vector<MemorySnapshot>(
        snapshots.end() - max_count, snapshots.end());
}

std::vector<MemorySnapshot> MemoryMonitor::getSnapshotsInRange(
    std::chrono::steady_clock::time_point start,
    std::chrono::steady_clock::time_point end) const {
    
    std::lock_guard<std::mutex> lock(snapshots_mutex);
    std::vector<MemorySnapshot> result;
    
    for (const auto& snapshot : snapshots) {
        if (snapshot.timestamp >= start && snapshot.timestamp <= end) {
            result.push_back(snapshot);
        }
    }
    
    return result;
}

MemorySnapshot MemoryMonitor::getAverageUsage() const {
    std::lock_guard<std::mutex> lock(snapshots_mutex);
    
    if (snapshots.empty()) {
        return MemorySnapshot();
    }
    
    MemorySnapshot avg;
    avg.resident_memory_mb = std::accumulate(snapshots.begin(), snapshots.end(), 0UL,
        [](size_t sum, const MemorySnapshot& s) { return sum + s.resident_memory_mb; }) / snapshots.size();
    
    avg.virtual_memory_mb = std::accumulate(snapshots.begin(), snapshots.end(), 0UL,
        [](size_t sum, const MemorySnapshot& s) { return sum + s.virtual_memory_mb; }) / snapshots.size();
    
    avg.cpu_usage_percent = std::accumulate(snapshots.begin(), snapshots.end(), 0.0,
        [](double sum, const MemorySnapshot& s) { return sum + s.cpu_usage_percent; }) / snapshots.size();
    
    return avg;
}

MemorySnapshot MemoryMonitor::getMaxUsage() const {
    std::lock_guard<std::mutex> lock(snapshots_mutex);
    
    if (snapshots.empty()) {
        return MemorySnapshot();
    }
    
    return *std::max_element(snapshots.begin(), snapshots.end(),
        [](const MemorySnapshot& a, const MemorySnapshot& b) {
            return a.resident_memory_mb < b.resident_memory_mb;
        });
}

MemorySnapshot MemoryMonitor::getMinUsage() const {
    std::lock_guard<std::mutex> lock(snapshots_mutex);
    
    if (snapshots.empty()) {
        return MemorySnapshot();
    }
    
    return *std::min_element(snapshots.begin(), snapshots.end(),
        [](const MemorySnapshot& a, const MemorySnapshot& b) {
            return a.resident_memory_mb < b.resident_memory_mb;
        });
}

void MemoryMonitor::setMemoryThreshold(size_t threshold_mb) {
    memory_threshold_mb = threshold_mb;
}

bool MemoryMonitor::isMemoryThresholdExceeded() const {
    return getCurrentMemoryMB() > memory_threshold_mb;
}

void MemoryMonitor::clearPeakMemory() {
    peak_memory_ever = getCurrentMemoryMB();
}

void MemoryMonitor::setSampleInterval(std::chrono::milliseconds interval) {
    sample_interval = interval;
}

void MemoryMonitor::setMaxSnapshots(size_t max_snaps) {
    max_snapshots = max_snaps;
    std::lock_guard<std::mutex> lock(snapshots_mutex);
    trimSnapshots();
}

bool MemoryMonitor::exportToCsv(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(snapshots_mutex);
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    // CSV header
    file << "Timestamp,Resident_MB,Virtual_MB,Peak_MB,Heap_MB,CPU_Percent\n";
    
    for (const auto& snapshot : snapshots) {
        file << formatTimestamp(snapshot.timestamp) << ","
             << snapshot.resident_memory_mb << ","
             << snapshot.virtual_memory_mb << ","
             << snapshot.peak_memory_mb << ","
             << snapshot.heap_memory_mb << ","
             << std::fixed << std::setprecision(2) << snapshot.cpu_usage_percent << "\n";
    }
    
    return true;
}

std::string MemoryMonitor::getMemoryReport() const {
    std::ostringstream report;
    
    MemorySnapshot current = getCurrentSnapshot();
    MemorySnapshot avg = getAverageUsage();
    MemorySnapshot max = getMaxUsage();
    
    report << "=== Memory Usage Report ===\n";
    report << "Current Memory: " << current.resident_memory_mb << " MB\n";
    report << "Peak Memory: " << peak_memory_ever << " MB\n";
    report << "Average Memory: " << avg.resident_memory_mb << " MB\n";
    report << "Maximum Recorded: " << max.resident_memory_mb << " MB\n";
    report << "Memory Threshold: " << memory_threshold_mb << " MB\n";
    report << "Threshold Exceeded: " << (isMemoryThresholdExceeded() ? "YES" : "NO") << "\n";
    report << "Total Snapshots: " << snapshots.size() << "\n";
    
    return report.str();
}

std::string MemoryMonitor::formatTimestamp(const std::chrono::steady_clock::time_point& tp) {
    auto time_since_epoch = tp.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(time_since_epoch);
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch) % 1000;
    
    std::ostringstream oss;
    oss << seconds.count() << "." << std::setfill('0') << std::setw(3) << milliseconds.count();
    return oss.str();
}