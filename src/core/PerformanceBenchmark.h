#ifndef PERFORMANCEBENCHMARK_H
#define PERFORMANCEBENCHMARK_H

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>

class PerformanceBenchmark {
public:
    struct BenchmarkResult {
        std::string operation_name;
        std::chrono::microseconds duration;
        size_t input_size;
        size_t output_size;
        std::chrono::steady_clock::time_point timestamp;
        
        BenchmarkResult(const std::string& name, 
                       std::chrono::microseconds dur,
                       size_t in_size = 0, 
                       size_t out_size = 0)
            : operation_name(name), duration(dur), input_size(in_size), 
              output_size(out_size), timestamp(std::chrono::steady_clock::now()) {}
    };

    struct Stats {
        double avg_ms;
        double min_ms;
        double max_ms;
        double std_dev_ms;
        size_t count;
    };

private:
    std::vector<BenchmarkResult> results;
    std::unordered_map<std::string, std::vector<std::chrono::microseconds>> operation_times;
    
public:
    PerformanceBenchmark() = default;
    
    // Timer class for RAII timing
    class Timer {
    private:
        std::chrono::steady_clock::time_point start_time;
        PerformanceBenchmark* benchmark;
        std::string operation_name;
        size_t input_size;
        size_t output_size;
        
    public:
        Timer(PerformanceBenchmark* bench, const std::string& name, size_t in_size = 0)
            : start_time(std::chrono::steady_clock::now()), 
              benchmark(bench), operation_name(name), input_size(in_size), output_size(0) {}
        
        ~Timer() {
            auto end_time = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            if (benchmark) {
                benchmark->addResult(operation_name, duration, input_size, output_size);
            }
        }
        
        void setOutputSize(size_t size) { output_size = size; }
    };
    
    // Add a benchmark result
    void addResult(const std::string& operation_name, 
                   std::chrono::microseconds duration,
                   size_t input_size = 0,
                   size_t output_size = 0);
    
    // Get statistics for a specific operation
    Stats getStats(const std::string& operation_name) const;
    
    // Get all results for an operation
    std::vector<BenchmarkResult> getResults(const std::string& operation_name) const;
    
    // Clear all results
    void clear();
    
    // Export results to CSV
    bool exportToCSV(const std::string& filename) const;
    
    // Print summary to console
    void printSummary() const;
    
    // Get list of all operation names
    std::vector<std::string> getOperationNames() const;
    
    // Measure memory usage (if available)
    static size_t getCurrentMemoryUsage();
    
    // Benchmark a function
    template<typename Func>
    auto benchmark(const std::string& name, Func&& func) -> decltype(func()) {
        Timer timer(this, name);
        return func();
    }
    
    // Benchmark with input/output size tracking
    template<typename Func>
    auto benchmarkWithSize(const std::string& name, size_t input_size, Func&& func) -> decltype(func()) {
        Timer timer(this, name, input_size);
        auto result = func();
        // If result is a container, set output size
        if constexpr (std::is_same_v<decltype(result), std::vector<std::string>>) {
            timer.setOutputSize(result.size());
        }
        return result;
    }

private:
    double calculateStandardDeviation(const std::vector<std::chrono::microseconds>& times, double mean) const;
};

// Global benchmark instance for easy access
extern PerformanceBenchmark g_benchmark;

// Convenience macros for benchmarking
#define BENCHMARK_SCOPE(name) PerformanceBenchmark::Timer _timer(&g_benchmark, name)
#define BENCHMARK_FUNCTION(name, func) g_benchmark.benchmark(name, func)

#endif // PERFORMANCEBENCHMARK_H