#include "PerformanceBenchmark.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#elif __APPLE__
#include <mach/mach.h>
#elif __linux__
#include <unistd.h>
#include <fstream>
#endif

// Global benchmark instance
PerformanceBenchmark g_benchmark;

void PerformanceBenchmark::addResult(const std::string& operation_name, 
                                    std::chrono::microseconds duration,
                                    size_t input_size,
                                    size_t output_size) {
    results.emplace_back(operation_name, duration, input_size, output_size);
    operation_times[operation_name].push_back(duration);
}

PerformanceBenchmark::Stats PerformanceBenchmark::getStats(const std::string& operation_name) const {
    auto it = operation_times.find(operation_name);
    if (it == operation_times.end() || it->second.empty()) {
        return {0.0, 0.0, 0.0, 0.0, 0};
    }
    
    const auto& times = it->second;
    
    // Calculate min, max, and average
    auto min_it = std::min_element(times.begin(), times.end());
    auto max_it = std::max_element(times.begin(), times.end());
    
    double sum_ms = 0.0;
    for (const auto& time : times) {
        sum_ms += time.count() / 1000.0; // Convert microseconds to milliseconds
    }
    
    double avg_ms = sum_ms / times.size();
    double min_ms = min_it->count() / 1000.0;
    double max_ms = max_it->count() / 1000.0;
    
    // Calculate standard deviation
    double std_dev_ms = calculateStandardDeviation(times, avg_ms * 1000.0) / 1000.0;
    
    return {avg_ms, min_ms, max_ms, std_dev_ms, times.size()};
}

std::vector<PerformanceBenchmark::BenchmarkResult> 
PerformanceBenchmark::getResults(const std::string& operation_name) const {
    std::vector<BenchmarkResult> filtered_results;
    
    for (const auto& result : results) {
        if (result.operation_name == operation_name) {
            filtered_results.push_back(result);
        }
    }
    
    return filtered_results;
}

void PerformanceBenchmark::clear() {
    results.clear();
    operation_times.clear();
}

bool PerformanceBenchmark::exportToCSV(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    // Write header
    file << "Operation,Duration_ms,Input_Size,Output_Size,Timestamp\n";
    
    // Write results
    for (const auto& result : results) {
        // Use duration cast to convert timestamps properly
        auto now_steady = std::chrono::steady_clock::now();
        auto now_system = std::chrono::system_clock::now();
        auto offset = std::chrono::duration_cast<std::chrono::system_clock::duration>(
            result.timestamp - now_steady);
        auto system_time = now_system + offset;
        auto time_t = std::chrono::system_clock::to_time_t(system_time);
        
        file << result.operation_name << ","
             << (result.duration.count() / 1000.0) << ","
             << result.input_size << ","
             << result.output_size << ","
             << time_t << "\n";
    }
    
    return true;
}

void PerformanceBenchmark::printSummary() const {
    if (operation_times.empty()) {
        std::cout << "No benchmark results available.\n";
        return;
    }
    
    std::cout << "\n=== Performance Benchmark Summary ===\n";
    std::cout << std::setw(20) << "Operation" 
              << std::setw(10) << "Count"
              << std::setw(12) << "Avg (ms)"
              << std::setw(12) << "Min (ms)"
              << std::setw(12) << "Max (ms)"
              << std::setw(12) << "StdDev (ms)" << "\n";
    std::cout << std::string(78, '-') << "\n";
    
    for (const auto& op_pair : operation_times) {
        const std::string& op_name = op_pair.first;
        Stats stats = getStats(op_name);
        
        std::cout << std::setw(20) << op_name
                  << std::setw(10) << stats.count
                  << std::setw(12) << std::fixed << std::setprecision(3) << stats.avg_ms
                  << std::setw(12) << std::fixed << std::setprecision(3) << stats.min_ms
                  << std::setw(12) << std::fixed << std::setprecision(3) << stats.max_ms
                  << std::setw(12) << std::fixed << std::setprecision(3) << stats.std_dev_ms << "\n";
    }
    
    std::cout << std::string(78, '-') << "\n";
    std::cout << "Total operations measured: " << results.size() << "\n";
    
    // Memory usage if available
    size_t memory_kb = getCurrentMemoryUsage();
    if (memory_kb > 0) {
        std::cout << "Current memory usage: " << (memory_kb / 1024.0) << " MB\n";
    }
    
    std::cout << std::endl;
}

std::vector<std::string> PerformanceBenchmark::getOperationNames() const {
    std::vector<std::string> names;
    names.reserve(operation_times.size());
    
    for (const auto& pair : operation_times) {
        names.push_back(pair.first);
    }
    
    std::sort(names.begin(), names.end());
    return names;
}

size_t PerformanceBenchmark::getCurrentMemoryUsage() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize / 1024; // Convert to KB
    }
#elif __APPLE__
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  (task_info_t)&info, &infoCount) == KERN_SUCCESS) {
        return info.resident_size / 1024; // Convert to KB
    }
#elif __linux__
    std::ifstream statm("/proc/self/statm");
    if (statm.is_open()) {
        size_t size, resident, share, text, lib, data, dt;
        statm >> size >> resident >> share >> text >> lib >> data >> dt;
        long page_size = sysconf(_SC_PAGESIZE);
        return (resident * page_size) / 1024; // Convert to KB
    }
#endif
    return 0; // Unable to determine memory usage
}

double PerformanceBenchmark::calculateStandardDeviation(
    const std::vector<std::chrono::microseconds>& times, double mean_microseconds) const {
    
    if (times.size() <= 1) {
        return 0.0;
    }
    
    double sum_squared_diff = 0.0;
    for (const auto& time : times) {
        double diff = time.count() - mean_microseconds;
        sum_squared_diff += diff * diff;
    }
    
    return std::sqrt(sum_squared_diff / (times.size() - 1));
}