#include "SearchCache.h"
#include <algorithm>

size_t SearchCache::generateKey(const std::string& query, const std::string& translation) const {
    // Combine query and translation into a single hash
    std::hash<std::string> hasher;
    size_t query_hash = hasher(query);
    size_t translation_hash = hasher(translation);
    
    // Combine hashes using boost-style hash combining
    return query_hash ^ (translation_hash + 0x9e3779b9 + (query_hash << 6) + (query_hash >> 2));
}

void SearchCache::evictOldest() const {
    if (lru_order.empty()) return;
    
    size_t oldest_key = lru_order.back();
    lru_order.pop_back();
    cache.erase(oldest_key);
}

void SearchCache::updateLRU(size_t key) const {
    // Remove key if it exists in LRU list
    auto it = std::find(lru_order.begin(), lru_order.end(), key);
    if (it != lru_order.end()) {
        lru_order.erase(it);
    }
    
    // Add to front (most recently used)
    lru_order.push_front(key);
}

bool SearchCache::isExpired(const CacheEntry& entry) const {
    auto now = std::chrono::steady_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::minutes>(now - entry.timestamp);
    return age > CACHE_TTL;
}

bool SearchCache::get(const std::string& query, const std::string& translation, 
                      std::vector<std::string>& results) const {
    size_t key = generateKey(query, translation);
    
    auto it = cache.find(key);
    if (it != cache.end()) {
        // Check if entry is expired
        if (isExpired(it->second)) {
            // Remove expired entry
            auto lru_it = std::find(lru_order.begin(), lru_order.end(), key);
            if (lru_it != lru_order.end()) {
                lru_order.erase(lru_it);
            }
            cache.erase(it);
            ++misses;
            return false;
        }
        
        // Cache hit - update LRU and return results
        results = it->second.results;
        updateLRU(key);
        ++hits;
        return true;
    }
    
    // Cache miss
    ++misses;
    return false;
}

void SearchCache::put(const std::string& query, const std::string& translation,
                      const std::vector<std::string>& results) const {
    size_t key = generateKey(query, translation);
    
    // Check if we need to evict entries to make room
    while (cache.size() >= MAX_CACHE_SIZE) {
        evictOldest();
    }
    
    // Add new entry
    cache[key] = CacheEntry(results);
    updateLRU(key);
}

void SearchCache::clear() {
    cache.clear();
    lru_order.clear();
    hits = 0;
    misses = 0;
}

double SearchCache::hitRate() const {
    size_t total = hits + misses;
    return total > 0 ? static_cast<double>(hits) / total : 0.0;
}

void SearchCache::cleanupExpired() {
    // Collect expired keys
    std::vector<size_t> expired_keys;
    for (const auto& pair : cache) {
        if (isExpired(pair.second)) {
            expired_keys.push_back(pair.first);
        }
    }
    
    // Remove expired entries
    for (size_t key : expired_keys) {
        cache.erase(key);
        auto lru_it = std::find(lru_order.begin(), lru_order.end(), key);
        if (lru_it != lru_order.end()) {
            lru_order.erase(lru_it);
        }
    }
}