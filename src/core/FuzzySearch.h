#ifndef FUZZYSEARCH_H
#define FUZZYSEARCH_H

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

struct FuzzyMatch {
    std::string text;
    double confidence;
    std::string matchType; // "exact", "fuzzy", "phonetic", "partial"
    
    FuzzyMatch() : text(""), confidence(0.0), matchType("") {}
    FuzzyMatch(const std::string& t, double c, const std::string& type)
        : text(t), confidence(c), matchType(type) {}
};

struct FuzzySearchOptions {
    bool enabled = true;
    double minConfidence = 0.6;
    bool enablePhonetic = true;
    bool enablePartialMatch = true;
    int maxSuggestions = 5;
    int maxEditDistance = 3;
    int maxCandidates = 1000;  // Limit candidates processed for performance
    bool enableEarlyTermination = true;  // Stop when good matches found
};

class FuzzySearch {
private:
    FuzzySearchOptions options;
    mutable std::unordered_map<std::string, std::string> soundexCache;
    mutable std::unordered_map<std::string, std::string> normalizeCache;
    
    // Levenshtein distance calculation
    int levenshteinDistance(const std::string& s1, const std::string& s2) const;
    
    // Soundex algorithm for phonetic matching
    std::string soundex(const std::string& word) const;
    
    // N-gram similarity calculation
    double ngramSimilarity(const std::string& s1, const std::string& s2, int n = 2) const;
    
    // Normalize text for comparison
    std::string normalize(const std::string& text) const;
    
    // Calculate confidence score based on various factors
    double calculateConfidence(const std::string& query, const std::string& target, int editDistance) const;

public:
    FuzzySearch();
    explicit FuzzySearch(const FuzzySearchOptions& opts);
    
    // Main fuzzy matching function
    std::vector<FuzzyMatch> findMatches(const std::string& query, const std::vector<std::string>& candidates) const;
    
    // Fuzzy book name matching
    std::vector<FuzzyMatch> findBookMatches(const std::string& query, const std::vector<std::string>& bookNames) const;
    
    // Single string fuzzy match
    FuzzyMatch calculateMatch(const std::string& query, const std::string& candidate) const;
    
    // Check if two strings are phonetically similar
    bool arePhoneticallySimilar(const std::string& s1, const std::string& s2) const;
    
    // Generate suggestions for misspelled queries
    std::vector<std::string> generateSuggestions(const std::string& query, const std::vector<std::string>& dictionary) const;
    
    // Configuration methods
    void setOptions(const FuzzySearchOptions& opts);
    const FuzzySearchOptions& getOptions() const;
    void setMinConfidence(double confidence);
    void enablePhonetic(bool enable);
    void enablePartialMatch(bool enable);
};

#endif // FUZZYSEARCH_H