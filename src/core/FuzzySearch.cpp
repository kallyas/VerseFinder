#include "FuzzySearch.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <set>
#include <unordered_set>

FuzzySearch::FuzzySearch() = default;

FuzzySearch::FuzzySearch(const FuzzySearchOptions& opts) : options(opts) {}

int FuzzySearch::levenshteinDistance(const std::string& s1, const std::string& s2) const {
    const size_t len1 = s1.length();
    const size_t len2 = s2.length();
    
    // Create a matrix for dynamic programming
    std::vector<std::vector<int>> dp(len1 + 1, std::vector<int>(len2 + 1));
    
    // Initialize base cases
    for (size_t i = 0; i <= len1; ++i) {
        dp[i][0] = i;
    }
    for (size_t j = 0; j <= len2; ++j) {
        dp[0][j] = j;
    }
    
    // Fill the matrix
    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            if (s1[i - 1] == s2[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                dp[i][j] = 1 + std::min({
                    dp[i - 1][j],     // deletion
                    dp[i][j - 1],     // insertion
                    dp[i - 1][j - 1]  // substitution
                });
            }
        }
    }
    
    return dp[len1][len2];
}

std::string FuzzySearch::soundex(const std::string& word) const {
    if (word.empty()) return "0000";
    
    // Check cache first
    auto it = soundexCache.find(word);
    if (it != soundexCache.end()) {
        return it->second;
    }
    
    std::string result;
    std::string normalized = normalize(word);
    
    if (normalized.empty()) {
        soundexCache[word] = "0000";
        return "0000";
    }
    
    // First character is always kept
    result += std::toupper(normalized[0]);
    
    // Soundex mapping
    std::string mapping = "01230120022455012623010202";
    std::string prev = "";
    
    for (size_t i = 1; i < normalized.length() && result.length() < 4; ++i) {
        char c = std::tolower(normalized[i]);
        if (c >= 'a' && c <= 'z') {
            size_t index = static_cast<size_t>(c - 'a');
            // Ensure index is within bounds of mapping string
            if (index < mapping.length()) {
                std::string code = mapping.substr(index, 1);
                if (code != "0" && code != prev) {
                    result += code;
                }
                prev = code;
            }
        }
    }
    
    // Pad with zeros
    while (result.length() < 4) {
        result += "0";
    }
    
    // Cache the result
    soundexCache[word] = result;
    return result;
}

double FuzzySearch::ngramSimilarity(const std::string& s1, const std::string& s2, int n) const {
    if (s1.empty() || s2.empty()) return 0.0;
    
    // Check if strings are too short for n-grams
    if (static_cast<int>(s1.length()) < n || static_cast<int>(s2.length()) < n) {
        return 0.0;
    }
    
    std::unordered_set<std::string> ngrams1, ngrams2;
    
    // Generate n-grams for both strings - safe bounds checking
    for (size_t i = 0; i <= s1.length() - static_cast<size_t>(n); ++i) {
        ngrams1.insert(s1.substr(i, n));
    }
    
    for (size_t i = 0; i <= s2.length() - static_cast<size_t>(n); ++i) {
        ngrams2.insert(s2.substr(i, n));
    }
    
    // Calculate Jaccard similarity
    std::unordered_set<std::string> intersection;
    for (const auto& ngram : ngrams1) {
        if (ngrams2.count(ngram)) {
            intersection.insert(ngram);
        }
    }
    
    size_t unionSize = ngrams1.size() + ngrams2.size() - intersection.size();
    return unionSize > 0 ? static_cast<double>(intersection.size()) / unionSize : 0.0;
}

std::string FuzzySearch::normalize(const std::string& text) const {
    std::string result;
    for (char c : text) {
        if (std::isalnum(c)) {
            result += std::tolower(c);
        }
    }
    return result;
}

double FuzzySearch::calculateConfidence(const std::string& query, const std::string& target, int editDistance) const {
    const std::string normQuery = normalize(query);
    const std::string normTarget = normalize(target);
    
    if (normQuery.empty() || normTarget.empty()) return 0.0;
    
    // Exact match gets perfect score
    if (normQuery == normTarget) return 1.0;
    
    // Calculate various similarity metrics
    const size_t maxLen = std::max(normQuery.length(), normTarget.length());
    const double editSimilarity = maxLen > 0 ? 1.0 - (static_cast<double>(editDistance) / maxLen) : 0.0;
    
    // N-gram similarity
    const double ngramSim = ngramSimilarity(normQuery, normTarget, 2);
    
    // Length difference penalty
    const double lengthDiff = std::abs(static_cast<int>(normQuery.length()) - static_cast<int>(normTarget.length()));
    const double lengthSimilarity = maxLen > 0 ? 1.0 - (lengthDiff / maxLen) : 0.0;
    
    // Combined confidence score
    const double confidence = (editSimilarity * 0.5) + (ngramSim * 0.3) + (lengthSimilarity * 0.2);
    
    return std::max(0.0, std::min(1.0, confidence));
}

std::vector<FuzzyMatch> FuzzySearch::findMatches(const std::string& query, const std::vector<std::string>& candidates) const {
    if (!options.enabled || query.empty()) return {};
    
    std::vector<FuzzyMatch> matches;
    const std::string normQuery = normalize(query);
    
    for (const auto& candidate : candidates) {
        FuzzyMatch match = calculateMatch(query, candidate);
        if (match.confidence >= options.minConfidence) {
            matches.push_back(match);
        }
    }
    
    // Sort by confidence (descending)
    std::sort(matches.begin(), matches.end(), [](const FuzzyMatch& a, const FuzzyMatch& b) {
        return a.confidence > b.confidence;
    });
    
    // Limit results
    if (matches.size() > static_cast<size_t>(options.maxSuggestions)) {
        matches.resize(options.maxSuggestions);
    }
    
    return matches;
}

std::vector<FuzzyMatch> FuzzySearch::findBookMatches(const std::string& query, const std::vector<std::string>& bookNames) const {
    if (!options.enabled || query.empty()) return {};
    
    std::vector<FuzzyMatch> matches;
    const std::string normQuery = normalize(query);
    
    for (const auto& bookName : bookNames) {
        const std::string normBook = normalize(bookName);
        
        // Check for exact match first
        if (normQuery == normBook || bookName.find(query) != std::string::npos) {
            matches.emplace_back(bookName, 1.0, "exact");
            continue;
        }
        
        // Calculate edit distance
        const int editDist = levenshteinDistance(normQuery, normBook);
        if (editDist <= options.maxEditDistance) {
            const double confidence = calculateConfidence(query, bookName, editDist);
            if (confidence >= options.minConfidence) {
                matches.emplace_back(bookName, confidence, "fuzzy");
            }
        }
        
        // Check phonetic similarity for book names
        if (options.enablePhonetic && arePhoneticallySimilar(query, bookName)) {
            const double phoneticConfidence = 0.8; // Base phonetic confidence
            matches.emplace_back(bookName, phoneticConfidence, "phonetic");
        }
        
        // Check partial matches (substring matching)
        if (options.enablePartialMatch) {
            if (normBook.find(normQuery) != std::string::npos || normQuery.find(normBook) != std::string::npos) {
                const double partialConfidence = 0.7; // Base partial match confidence
                matches.emplace_back(bookName, partialConfidence, "partial");
            }
        }
    }
    
    // Remove duplicates and sort by confidence
    std::sort(matches.begin(), matches.end(), [](const FuzzyMatch& a, const FuzzyMatch& b) {
        if (a.text == b.text) return a.confidence > b.confidence;
        return a.confidence > b.confidence;
    });
    
    auto last = std::unique(matches.begin(), matches.end(), 
        [](const FuzzyMatch& a, const FuzzyMatch& b) { return a.text == b.text; });
    matches.erase(last, matches.end());
    
    // Limit results
    if (matches.size() > static_cast<size_t>(options.maxSuggestions)) {
        matches.resize(options.maxSuggestions);
    }
    
    return matches;
}

FuzzyMatch FuzzySearch::calculateMatch(const std::string& query, const std::string& candidate) const {
    const std::string normQuery = normalize(query);
    const std::string normCandidate = normalize(candidate);
    
    // Exact match
    if (normQuery == normCandidate) {
        return FuzzyMatch(candidate, 1.0, "exact");
    }
    
    // Calculate edit distance
    const int editDist = levenshteinDistance(normQuery, normCandidate);
    const double confidence = calculateConfidence(query, candidate, editDist);
    
    // Determine match type
    std::string matchType = "fuzzy";
    if (options.enablePhonetic && arePhoneticallySimilar(query, candidate)) {
        matchType = "phonetic";
    } else if (options.enablePartialMatch && 
               (normCandidate.find(normQuery) != std::string::npos || 
                normQuery.find(normCandidate) != std::string::npos)) {
        matchType = "partial";
    }
    
    return FuzzyMatch(candidate, confidence, matchType);
}

bool FuzzySearch::arePhoneticallySimilar(const std::string& s1, const std::string& s2) const {
    if (!options.enablePhonetic) return false;
    
    const std::string sound1 = soundex(s1);
    const std::string sound2 = soundex(s2);
    
    return sound1 == sound2 && sound1 != "0000";
}

std::vector<std::string> FuzzySearch::generateSuggestions(const std::string& query, const std::vector<std::string>& dictionary) const {
    if (!options.enabled || query.empty()) return {};
    
    std::vector<FuzzyMatch> matches = findMatches(query, dictionary);
    std::vector<std::string> suggestions;
    
    for (const auto& match : matches) {
        if (match.matchType != "exact") { // Only suggest alternatives, not exact matches
            suggestions.push_back(match.text);
        }
    }
    
    return suggestions;
}

void FuzzySearch::setOptions(const FuzzySearchOptions& opts) {
    options = opts;
}

const FuzzySearchOptions& FuzzySearch::getOptions() const {
    return options;
}

void FuzzySearch::setMinConfidence(double confidence) {
    options.minConfidence = std::max(0.0, std::min(1.0, confidence));
}

void FuzzySearch::enablePhonetic(bool enable) {
    options.enablePhonetic = enable;
}

void FuzzySearch::enablePartialMatch(bool enable) {
    options.enablePartialMatch = enable;
}