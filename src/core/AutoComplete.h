#ifndef AUTOCOMPLETE_H
#define AUTOCOMPLETE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <algorithm>
#include <memory>

struct TrieNode {
    std::unordered_map<char, std::unique_ptr<TrieNode>> children;
    std::vector<std::string> completions;
    bool is_end_of_word = false;
    
    // Limit completions per node for memory efficiency
    static constexpr size_t MAX_COMPLETIONS = 10;
    
    void addCompletion(const std::string& word) {
        if (completions.size() < MAX_COMPLETIONS) {
            // Insert in sorted order for faster retrieval
            auto it = std::lower_bound(completions.begin(), completions.end(), word);
            if (it == completions.end() || *it != word) {
                completions.insert(it, word);
            }
        }
    }
};

class AutoComplete {
private:
    std::unique_ptr<TrieNode> root;
    std::unordered_map<std::string, std::set<std::string>> reference_patterns;
    std::unordered_map<std::string, int> word_frequency;
    
    // Cache for recent searches to improve performance
    mutable std::unordered_map<std::string, std::vector<std::string>> suggestion_cache;
    static constexpr size_t MAX_CACHE_SIZE = 1000;
    
    void insertWord(const std::string& word);
    void collectCompletions(TrieNode* node, const std::string& prefix, 
                           std::vector<std::string>& results, int max_results) const;
    
    // Reference pattern matching (e.g., "John 3:16", "Genesis 1")
    bool isReferencePattern(const std::string& input) const;
    std::vector<std::string> generateReferenceCompletions(const std::string& input) const;
    
    // Keyword-based completions
    std::vector<std::string> generateKeywordCompletions(const std::string& input) const;

public:
    AutoComplete();
    ~AutoComplete() = default;
    
    // Non-copyable for performance
    AutoComplete(const AutoComplete&) = delete;
    AutoComplete& operator=(const AutoComplete&) = delete;
    
    // Build the autocomplete index from verse data
    void buildIndex(const std::unordered_map<std::string, std::unordered_map<std::string, class Verse>>& verses);
    
    // Add a book name to the reference patterns
    void addBookName(const std::string& book_name);
    
    // Get completions for a given input string
    std::vector<std::string> getCompletions(const std::string& input, int max_results = 10) const;
    
    // Get smart suggestions that consider context and frequency
    std::vector<std::string> getSmartSuggestions(const std::string& input, int max_results = 10) const;
    
    // Clear the suggestion cache
    void clearCache();
    
    // Update word frequency for better ranking
    void updateWordFrequency(const std::string& word);
    
    // Get memory usage statistics
    size_t getMemoryUsage() const;
    
    // Clear all data
    void clear();
    
private:
    // Helper methods for building the index
    void addVerseText(const std::string& text);
    void addReferencePattern(const std::string& book, int chapter, int verse);
    
    // Scoring and ranking functions
    double calculateWordScore(const std::string& word, const std::string& input) const;
    void rankSuggestions(std::vector<std::string>& suggestions, const std::string& input) const;
    
    // Cache management
    void manageCacheSize() const;
};

#endif // AUTOCOMPLETE_H