#ifndef SEARCHCACHE_H
#define SEARCHCACHE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <list>
#include <chrono>
#include <functional>

class SearchCache {
public:
    struct CacheEntry {
        std::vector<std::string> results;
        std::chrono::steady_clock::time_point timestamp;
        
        CacheEntry() = default;
        CacheEntry(const std::vector<std::string>& res) 
            : results(res), timestamp(std::chrono::steady_clock::now()) {}
    };

private:
    mutable std::unordered_map<size_t, CacheEntry> cache;
    mutable std::list<size_t> lru_order;
    static constexpr size_t MAX_CACHE_SIZE = 200;
    static constexpr std::chrono::minutes CACHE_TTL{30}; // 30 minutes TTL

    size_t generateKey(const std::string& query, const std::string& translation) const;
    void evictOldest() const;
    void updateLRU(size_t key) const;
    bool isExpired(const CacheEntry& entry) const;

public:
    SearchCache() = default;
    ~SearchCache() = default;

    // Non-copyable
    SearchCache(const SearchCache&) = delete;
    SearchCache& operator=(const SearchCache&) = delete;

    bool get(const std::string& query, const std::string& translation, 
             std::vector<std::string>& results) const;
    
    void put(const std::string& query, const std::string& translation,
             const std::vector<std::string>& results) const;
    
    void clear();
    
    // Statistics and management
    size_t size() const { return cache.size(); }
    size_t maxSize() const { return MAX_CACHE_SIZE; }
    double hitRate() const;
    void cleanupExpired();

private:
    // Statistics tracking
    mutable size_t hits = 0;
    mutable size_t misses = 0;
};

#endif // SEARCHCACHE_H