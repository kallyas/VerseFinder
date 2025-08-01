#ifndef INCREMENTALSEARCH_H
#define INCREMENTALSEARCH_H

#include <string>
#include <vector>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>

class VerseFinder; // Forward declaration

struct SearchRequest {
    std::string query;
    std::string translation;
    std::chrono::steady_clock::time_point timestamp;
    int request_id;
    
    SearchRequest(const std::string& q, const std::string& t, int id)
        : query(q), translation(t), timestamp(std::chrono::steady_clock::now()), request_id(id) {}
};

struct SearchResult {
    std::vector<std::string> results;
    std::string query;
    std::string translation;
    int request_id;
    std::chrono::steady_clock::time_point timestamp;
    std::chrono::microseconds search_duration;
    
    SearchResult(const std::vector<std::string>& res, const std::string& q, 
                const std::string& t, int id, std::chrono::microseconds duration)
        : results(res), query(q), translation(t), request_id(id),
          timestamp(std::chrono::steady_clock::now()), search_duration(duration) {}
};

class IncrementalSearch {
public:
    using ResultCallback = std::function<void(const SearchResult&)>;
    
private:
    VerseFinder* verse_finder;
    std::atomic<bool> running{false};
    std::thread search_thread;
    
    // Search queue and synchronization
    std::queue<SearchRequest> search_queue;
    mutable std::mutex queue_mutex;
    std::condition_variable queue_condition;
    
    // Configuration
    std::chrono::milliseconds debounce_delay{150}; // Wait 150ms after last keystroke
    std::chrono::milliseconds max_search_time{50}; // Target max search time
    size_t max_queue_size = 10;
    
    // Callback for results
    ResultCallback result_callback;
    
    // Request tracking
    std::atomic<int> next_request_id{1};
    std::atomic<int> last_completed_id{0};
    
    // Performance tracking
    mutable std::mutex stats_mutex;
    size_t total_searches = 0;
    std::chrono::microseconds total_search_time{0};
    std::chrono::microseconds fastest_search{std::chrono::microseconds::max()};
    std::chrono::microseconds slowest_search{0};
    
    void searchWorkerLoop();
    void processSearchRequest(const SearchRequest& request);
    bool shouldCancelSearch(int current_id) const;
    
public:
    explicit IncrementalSearch(VerseFinder* vf);
    ~IncrementalSearch();
    
    // Non-copyable
    IncrementalSearch(const IncrementalSearch&) = delete;
    IncrementalSearch& operator=(const IncrementalSearch&) = delete;
    
    // Control
    void start();
    void stop();
    bool isRunning() const { return running.load(); }
    
    // Configuration
    void setDebounceDelay(std::chrono::milliseconds delay) { debounce_delay = delay; }
    void setMaxSearchTime(std::chrono::milliseconds time) { max_search_time = time; }
    void setMaxQueueSize(size_t size) { max_queue_size = size; }
    void setResultCallback(ResultCallback callback) { result_callback = std::move(callback); }
    
    // Search operations
    int submitSearch(const std::string& query, const std::string& translation);
    void cancelAllSearches();
    
    // Statistics
    struct SearchStats {
        size_t total_searches;
        double average_search_time_ms;
        double fastest_search_ms;
        double slowest_search_ms;
        size_t queue_size;
        bool is_running;
    };
    
    SearchStats getStats() const;
    void resetStats();
    
    // Queue management
    size_t getQueueSize() const;
    void clearQueue();
};

#endif // INCREMENTALSEARCH_H