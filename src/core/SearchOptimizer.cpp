#include "SearchOptimizer.h"
#include <algorithm>
#include <cctype>

std::vector<std::string> SearchOptimizer::optimizedIntersection(
    const std::vector<std::vector<std::string>>& token_lists) {
    
    if (token_lists.empty()) {
        return {};
    }
    
    if (token_lists.size() == 1) {
        return token_lists[0];
    }
    
    // Early termination if any list is empty
    for (const auto& list : token_lists) {
        if (list.empty()) {
            return {};
        }
    }
    
    // Sort lists by size for optimal intersection order
    auto sorted_lists = sortTokenListsBySize(token_lists);
    
    // Use iterative intersection starting with smallest list
    return multiWayIntersection(sorted_lists);
}

std::vector<std::string> SearchOptimizer::intersectTwo(
    const std::vector<std::string>& list1,
    const std::vector<std::string>& list2) {
    
    std::vector<std::string> result;
    
    // Choose algorithm based on list sizes
    if (list1.size() > BINARY_SEARCH_THRESHOLD && list2.size() > BINARY_SEARCH_THRESHOLD) {
        // Use standard set intersection for large sorted lists
        std::set_intersection(list1.begin(), list1.end(),
                             list2.begin(), list2.end(),
                             std::back_inserter(result));
    } else if (list1.size() * 10 < list2.size()) {
        // If one list is much smaller, use binary search in larger list
        const auto& small_list = list1;
        const auto& large_list = list2;
        
        result.reserve(std::min(small_list.size(), large_list.size() / 10));
        
        for (const auto& item : small_list) {
            if (binarySearchInVector(large_list, item)) {
                result.push_back(item);
            }
        }
    } else if (list2.size() * 10 < list1.size()) {
        // Reverse case
        const auto& small_list = list2;
        const auto& large_list = list1;
        
        result.reserve(std::min(small_list.size(), large_list.size() / 10));
        
        for (const auto& item : small_list) {
            if (binarySearchInVector(large_list, item)) {
                result.push_back(item);
            }
        }
    } else {
        // Similar sizes, use standard intersection
        result.reserve(std::min(list1.size(), list2.size()));
        std::set_intersection(list1.begin(), list1.end(),
                             list2.begin(), list2.end(),
                             std::back_inserter(result));
    }
    
    return result;
}

std::vector<std::vector<std::string>> SearchOptimizer::sortTokenListsBySize(
    const std::vector<std::vector<std::string>>& token_lists) {
    
    auto sorted_lists = token_lists;
    
    // Sort by size (smallest first)
    std::sort(sorted_lists.begin(), sorted_lists.end(),
              [](const std::vector<std::string>& a, const std::vector<std::string>& b) {
                  return a.size() < b.size();
              });
    
    return sorted_lists;
}

std::string SearchOptimizer::preprocessToken(const std::string& token) {
    std::string result;
    result.reserve(token.size());
    
    for (char c : token) {
        if (std::isalnum(c)) {
            result += std::tolower(c);
        }
    }
    
    return result;
}

std::vector<std::string> SearchOptimizer::optimizedTokenize(const std::string& text) {
    std::vector<std::string> tokens;
    tokens.reserve(text.size() / 5); // Estimate average token length
    
    std::string token;
    token.reserve(20); // Reserve space for typical word length
    
    for (char c : text) {
        if (std::isalnum(c)) {
            token += std::tolower(c);
        } else if (!token.empty()) {
            tokens.push_back(std::move(token));
            token.clear();
            token.reserve(20);
        }
    }
    
    if (!token.empty()) {
        tokens.push_back(std::move(token));
    }
    
    return tokens;
}

bool SearchOptimizer::verifyPhraseMatch(const std::string& text, const std::string& query) {
    // Convert both to lowercase for comparison
    std::string lower_text = text;
    std::string lower_query = query;
    
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    size_t pos = lower_text.find(lower_query);
    if (pos == std::string::npos) {
        return false;
    }
    
    // Verify word boundaries for exact phrase matching
    bool word_start = (pos == 0 || !std::isalnum(lower_text[pos - 1]));
    bool word_end = (pos + lower_query.length() >= lower_text.length() || 
                     !std::isalnum(lower_text[pos + lower_query.length()]));
    
    return word_start && word_end;
}

size_t SearchOptimizer::estimateIntersectionSize(
    const std::vector<std::vector<std::string>>& token_lists) {
    
    if (token_lists.empty()) {
        return 0;
    }
    
    // Estimate based on smallest list size
    size_t min_size = token_lists[0].size();
    for (const auto& list : token_lists) {
        min_size = std::min(min_size, list.size());
    }
    
    // Conservative estimate: assume 10% overlap between each pair
    double estimated_size = min_size;
    for (size_t i = 1; i < token_lists.size(); ++i) {
        estimated_size *= 0.1; // 10% overlap assumption
    }
    
    return static_cast<size_t>(std::max(1.0, estimated_size));
}

std::vector<std::string> SearchOptimizer::multiWayIntersection(
    std::vector<std::vector<std::string>>& sorted_lists) {
    
    if (sorted_lists.empty()) {
        return {};
    }
    
    // Start with smallest list
    std::vector<std::string> result = std::move(sorted_lists[0]);
    
    // Intersect with each subsequent list
    for (size_t i = 1; i < sorted_lists.size(); ++i) {
        if (result.empty()) {
            break; // Early termination
        }
        
        result = intersectTwo(result, sorted_lists[i]);
    }
    
    return result;
}

bool SearchOptimizer::binarySearchInVector(const std::vector<std::string>& vec, 
                                          const std::string& target) {
    return std::binary_search(vec.begin(), vec.end(), target);
}