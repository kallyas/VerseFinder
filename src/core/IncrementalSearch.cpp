#include "IncrementalSearch.h"
#include "VerseFinder.h"
#include <algorithm>
#include <iostream>

IncrementalSearch::IncrementalSearch(VerseFinder* vf) : verse_finder(vf) {
}

IncrementalSearch::~IncrementalSearch() {
    stop();
}

void IncrementalSearch::start() {
    if (running.load()) {
        return; // Already running
    }
    
    running.store(true);
    search_thread = std::thread(&IncrementalSearch::searchWorkerLoop, this);
}

void IncrementalSearch::stop() {
    if (!running.load()) {
        return; // Not running
    }
    
    running.store(false);
    queue_condition.notify_all();
    
    if (search_thread.joinable()) {
        search_thread.join();
    }
    
    clearQueue();
}

void IncrementalSearch::searchWorkerLoop() {
    std::unique_lock<std::mutex> lock(queue_mutex);
    
    while (running.load()) {
        // Wait for a search request or stop signal
        queue_condition.wait(lock, [this] {
            return !search_queue.empty() || !running.load();
        });
        
        if (!running.load()) {
            break;
        }
        
        if (search_queue.empty()) {
            continue;
        }
        
        // Get the most recent search request and discard older ones
        SearchRequest request = search_queue.back();
        
        // Clear the queue to process only the latest request
        while (!search_queue.empty()) {
            search_queue.pop();
        }
        
        lock.unlock();
        
        // Check if enough time has passed since the request (debouncing)
        auto now = std::chrono::steady_clock::now();
        auto time_since_request = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - request.timestamp);
        
        if (time_since_request < debounce_delay) {
            // Wait for the remaining debounce time
            std::this_thread::sleep_for(debounce_delay - time_since_request);
            
            // Check if a newer request has arrived while waiting
            lock.lock();
            if (!search_queue.empty()) {
                continue; // Process the newer request instead
            }
            lock.unlock();
        }
        
        // Process the search request
        processSearchRequest(request);
        
        lock.lock();
    }
}

void IncrementalSearch::processSearchRequest(const SearchRequest& request) {
    if (!verse_finder || !verse_finder->isReady()) {
        return;
    }
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Perform the search based on query type
    std::vector<std::string> results;
    
    try {
        if (request.query.empty()) {
            // Empty query - return empty results
            results = {};
        } else {
            // Determine search type and perform search
            std::string book;
            int chapter, verse;
            if (verse_finder->parseReference(request.query, book, chapter, verse)) {
                // Reference search
                std::string verse_result = verse_finder->searchByReference(request.query, request.translation);
                if (!verse_result.empty()) {
                    results.push_back(verse_result);
                }
            } else {
                // Keyword search
                results = verse_finder->searchByKeywords(request.query, request.translation);
                
                // Limit results for performance
                if (results.size() > 50) {
                    results.resize(50);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in incremental search: " << e.what() << std::endl;
        results = {}; // Return empty results on error
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto search_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // Update statistics
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex);
        total_searches++;
        total_search_time += search_duration;
        fastest_search = std::min(fastest_search, search_duration);
        slowest_search = std::max(slowest_search, search_duration);
    }
    
    // Check if this search was cancelled by a newer request
    if (shouldCancelSearch(request.request_id)) {
        return;
    }
    
    // Update last completed ID
    last_completed_id.store(request.request_id);
    
    // Call the result callback if set
    if (result_callback) {
        SearchResult result(results, request.query, request.translation, 
                          request.request_id, search_duration);
        result_callback(result);
    }
}

bool IncrementalSearch::shouldCancelSearch(int current_id) const {
    // Cancel if there are newer requests in the queue
    std::lock_guard<std::mutex> lock(queue_mutex);
    return !search_queue.empty() && search_queue.back().request_id > current_id;
}

int IncrementalSearch::submitSearch(const std::string& query, const std::string& translation) {
    if (!running.load()) {
        return -1;
    }
    
    int request_id = next_request_id.fetch_add(1);
    SearchRequest request(query, translation, request_id);
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        
        // Limit queue size to prevent memory buildup
        while (search_queue.size() >= max_queue_size) {
            search_queue.pop();
        }
        
        search_queue.push(request);
    }
    
    queue_condition.notify_one();
    return request_id;
}

void IncrementalSearch::cancelAllSearches() {
    clearQueue();
}

IncrementalSearch::SearchStats IncrementalSearch::getStats() const {
    std::lock_guard<std::mutex> stats_lock(stats_mutex);
    
    SearchStats stats;
    stats.total_searches = total_searches;
    stats.queue_size = getQueueSize();
    stats.is_running = running.load();
    
    if (total_searches > 0) {
        stats.average_search_time_ms = static_cast<double>(total_search_time.count()) / 
                                     (total_searches * 1000.0);
        stats.fastest_search_ms = fastest_search.count() / 1000.0;
        stats.slowest_search_ms = slowest_search.count() / 1000.0;
    } else {
        stats.average_search_time_ms = 0.0;
        stats.fastest_search_ms = 0.0;
        stats.slowest_search_ms = 0.0;
    }
    
    return stats;
}

void IncrementalSearch::resetStats() {
    std::lock_guard<std::mutex> stats_lock(stats_mutex);
    total_searches = 0;
    total_search_time = std::chrono::microseconds{0};
    fastest_search = std::chrono::microseconds::max();
    slowest_search = std::chrono::microseconds{0};
}

size_t IncrementalSearch::getQueueSize() const {
    std::lock_guard<std::mutex> lock(queue_mutex);
    return search_queue.size();
}

void IncrementalSearch::clearQueue() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    while (!search_queue.empty()) {
        search_queue.pop();
    }
}