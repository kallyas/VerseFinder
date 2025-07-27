#ifndef SEARCHOPTIMIZER_H
#define SEARCHOPTIMIZER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

class SearchOptimizer {
public:
    // Optimized intersection algorithm for sorted vectors
    static std::vector<std::string> optimizedIntersection(
        const std::vector<std::vector<std::string>>& token_lists);
    
    // Fast intersection for two sorted vectors
    static std::vector<std::string> intersectTwo(
        const std::vector<std::string>& list1,
        const std::vector<std::string>& list2);
    
    // Sort token lists by size (smallest first) for optimal intersection
    static std::vector<std::vector<std::string>> sortTokenListsBySize(
        const std::vector<std::vector<std::string>>& token_lists);
    
    // Pre-process tokens to lowercase during indexing
    static std::string preprocessToken(const std::string& token);
    
    // Optimized tokenization with reserved capacity
    static std::vector<std::string> optimizedTokenize(const std::string& text);
    
    // Check if query contains phrase and verify word boundaries
    static bool verifyPhraseMatch(const std::string& text, const std::string& query);
    
    // Calculate estimated result size for early termination
    static size_t estimateIntersectionSize(const std::vector<std::vector<std::string>>& token_lists);

private:
    // Helper for multi-way intersection using iterative approach
    static std::vector<std::string> multiWayIntersection(
        std::vector<std::vector<std::string>>& sorted_lists);
    
    // Binary search optimization for large lists
    static bool binarySearchInVector(const std::vector<std::string>& vec, const std::string& target);
    
    // Threshold for switching to binary search
    static constexpr size_t BINARY_SEARCH_THRESHOLD = 100;
};

#endif // SEARCHOPTIMIZER_H