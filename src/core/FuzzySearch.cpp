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
    
    // Early exit for empty strings
    if (len1 == 0) return len2;
    if (len2 == 0) return len1;
    
    // Early exit if difference in length is too large
    if (std::abs(static_cast<int>(len1) - static_cast<int>(len2)) > options.maxEditDistance) {
        return options.maxEditDistance + 1;
    }
    
    // Use space-optimized version with only two rows instead of full matrix
    std::vector<int> prev_row(len2 + 1);
    std::vector<int> curr_row(len2 + 1);
    
    // Initialize first row
    for (size_t j = 0; j <= len2; ++j) {
        prev_row[j] = j;
    }
    
    // Fill the matrix row by row
    for (size_t i = 1; i <= len1; ++i) {
        curr_row[0] = i;
        
        // Early termination check - if minimum value in row exceeds threshold
        int min_in_row = curr_row[0];
        
        for (size_t j = 1; j <= len2; ++j) {
            if (s1[i - 1] == s2[j - 1]) {
                curr_row[j] = prev_row[j - 1];
            } else {
                curr_row[j] = 1 + std::min({
                    prev_row[j],      // deletion
                    curr_row[j - 1],  // insertion
                    prev_row[j - 1]   // substitution
                });
            }
            min_in_row = std::min(min_in_row, curr_row[j]);
        }
        
        // Early termination if minimum distance exceeds threshold
        if (min_in_row > options.maxEditDistance) {
            return options.maxEditDistance + 1;
        }
        
        // Swap rows
        prev_row.swap(curr_row);
    }
    
    return prev_row[len2];
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
    
    // For short strings, use a simpler approach
    if (s1.length() + s2.length() < 10) {
        return s1 == s2 ? 1.0 : 0.0;
    }
    
    // Use vectors instead of sets for better performance with small data
    std::vector<std::string> ngrams1, ngrams2;
    ngrams1.reserve(s1.length() - n + 1);
    ngrams2.reserve(s2.length() - n + 1);
    
    // Generate n-grams for both strings - safe bounds checking
    for (size_t i = 0; i <= s1.length() - static_cast<size_t>(n); ++i) {
        ngrams1.push_back(s1.substr(i, n));
    }
    
    for (size_t i = 0; i <= s2.length() - static_cast<size_t>(n); ++i) {
        ngrams2.push_back(s2.substr(i, n));
    }
    
    // Sort for efficient intersection
    std::sort(ngrams1.begin(), ngrams1.end());
    std::sort(ngrams2.begin(), ngrams2.end());
    
    // Count intersection
    std::vector<std::string> intersection;
    std::set_intersection(ngrams1.begin(), ngrams1.end(),
                         ngrams2.begin(), ngrams2.end(),
                         std::back_inserter(intersection));
    
    size_t unionSize = ngrams1.size() + ngrams2.size() - intersection.size();
    return unionSize > 0 ? static_cast<double>(intersection.size()) / unionSize : 0.0;
}

std::string FuzzySearch::normalize(const std::string& text) const {
    // Check cache first for frequently used strings
    auto it = normalizeCache.find(text);
    if (it != normalizeCache.end()) {
        return it->second;
    }
    
    std::string result;
    result.reserve(text.length()); // Reserve space to avoid reallocations
    
    for (char c : text) {
        if (std::isalnum(c)) {
            result += std::tolower(c);
        }
    }
    
    // Cache the result if cache isn't too large
    if (normalizeCache.size() < 1000) {
        normalizeCache[text] = result;
    }
    
    return result;
}

double FuzzySearch::calculateConfidence(const std::string& query, const std::string& target, int editDistance) const {
    const std::string normQuery = normalize(query);
    const std::string normTarget = normalize(target);
    
    if (normQuery.empty() || normTarget.empty()) return 0.0;
    
    // Exact match gets perfect score
    if (normQuery == normTarget) return 1.0;
    
    // Quick reject for very high edit distances
    const size_t maxLen = std::max(normQuery.length(), normTarget.length());
    if (editDistance > static_cast<int>(maxLen) / 2) return 0.0;
    
    // Calculate basic edit similarity
    const double editSimilarity = maxLen > 0 ? 1.0 - (static_cast<double>(editDistance) / maxLen) : 0.0;
    
    // If edit similarity is already very low, skip expensive n-gram calculation
    if (editSimilarity < 0.3) {
        return editSimilarity;
    }
    
    // Length difference penalty
    const double lengthDiff = std::abs(static_cast<int>(normQuery.length()) - static_cast<int>(normTarget.length()));
    const double lengthSimilarity = maxLen > 0 ? 1.0 - (lengthDiff / maxLen) : 0.0;
    
    // Only calculate n-gram similarity for promising candidates
    double ngramSim = 0.0;
    if (editSimilarity > 0.5 || lengthSimilarity > 0.7) {
        ngramSim = ngramSimilarity(normQuery, normTarget, 2);
    }
    
    // Combined confidence score with weighted components
    const double confidence = (editSimilarity * 0.6) + (ngramSim * 0.25) + (lengthSimilarity * 0.15);
    
    return std::max(0.0, std::min(1.0, confidence));
}

std::vector<FuzzyMatch> FuzzySearch::findMatches(const std::string& query, const std::vector<std::string>& candidates) const {
    if (!options.enabled || query.empty()) return {};
    
    std::vector<FuzzyMatch> matches;
    matches.reserve(std::min(candidates.size(), static_cast<size_t>(options.maxSuggestions * 2))); // Reserve space
    
    const std::string normQuery = normalize(query);
    
    // Early termination counters
    int exact_matches = 0;
    int good_matches = 0;
    
    for (const auto& candidate : candidates) {
        // Quick filter: skip candidates that are obviously too different
        const std::string normCandidate = normalize(candidate);
        
        // Skip if length difference is too large (quick filter)
        if (std::abs(static_cast<int>(normQuery.length()) - static_cast<int>(normCandidate.length())) > options.maxEditDistance * 2) {
            continue;
        }
        
        FuzzyMatch match = calculateMatch(query, candidate);
        if (match.confidence >= options.minConfidence) {
            matches.push_back(match);
            
            // Track quality matches for early termination
            if (match.matchType == "exact") {
                exact_matches++;
                if (exact_matches >= 2) break; // Stop early if we have exact matches
            } else if (match.confidence > 0.9) {
                good_matches++;
                if (good_matches >= 3) break; // Stop if we have excellent matches
            } else if (match.confidence > 0.8) {
                good_matches++;
                if (good_matches >= options.maxSuggestions) break; // Stop if we have enough good matches
            }
        }
        
        // Hard limit to prevent excessive processing - be more aggressive
        if (matches.size() >= static_cast<size_t>(options.maxSuggestions * 2)) break;
    }
    
    // Sort by confidence (descending) - use partial sort for better performance
    if (matches.size() > static_cast<size_t>(options.maxSuggestions)) {
        std::partial_sort(matches.begin(), matches.begin() + options.maxSuggestions, matches.end(),
                         [](const FuzzyMatch& a, const FuzzyMatch& b) {
                             return a.confidence > b.confidence;
                         });
        matches.resize(options.maxSuggestions);
    } else {
        std::sort(matches.begin(), matches.end(), [](const FuzzyMatch& a, const FuzzyMatch& b) {
            return a.confidence > b.confidence;
        });
    }
    
    return matches;
}

std::vector<FuzzyMatch> FuzzySearch::findBookMatches(const std::string& query, const std::vector<std::string>& bookNames) const {
    if (!options.enabled || query.empty()) return {};
    
    std::vector<FuzzyMatch> matches;
    matches.reserve(bookNames.size()); // Reserve space since book names are limited
    
    const std::string normQuery = normalize(query);
    
    // Quick exact and partial match pass first
    for (const auto& bookName : bookNames) {
        const std::string normBook = normalize(bookName);
        
        // Check for exact match first
        if (normQuery == normBook || bookName.find(query) != std::string::npos) {
            matches.emplace_back(bookName, 1.0, "exact");
            continue;
        }
        
        // Quick partial match check
        if (options.enablePartialMatch) {
            if (normBook.find(normQuery) != std::string::npos) {
                double confidence = static_cast<double>(normQuery.length()) / normBook.length();
                matches.emplace_back(bookName, std::min(0.9, confidence + 0.2), "partial");
                continue;
            }
            if (normQuery.find(normBook) != std::string::npos) {
                double confidence = static_cast<double>(normBook.length()) / normQuery.length();
                matches.emplace_back(bookName, std::min(0.9, confidence + 0.2), "partial");
                continue;
            }
        }
    }
    
    // If we found good matches, skip expensive fuzzy search
    if (!matches.empty() && matches[0].confidence >= 0.8) {
        // Sort by confidence and return early
        std::sort(matches.begin(), matches.end(), [](const FuzzyMatch& a, const FuzzyMatch& b) {
            if (a.text == b.text) return a.confidence > b.confidence;
            return a.confidence > b.confidence;
        });
        
        // Remove duplicates
        auto last = std::unique(matches.begin(), matches.end(), 
            [](const FuzzyMatch& a, const FuzzyMatch& b) { return a.text == b.text; });
        matches.erase(last, matches.end());
        
        if (matches.size() > static_cast<size_t>(options.maxSuggestions)) {
            matches.resize(options.maxSuggestions);
        }
        return matches;
    }
    
    // Now do fuzzy search only if needed
    for (const auto& bookName : bookNames) {
        const std::string normBook = normalize(bookName);
        
        // Skip if already found as exact/partial match
        bool already_found = false;
        for (const auto& existing : matches) {
            if (existing.text == bookName) {
                already_found = true;
                break;
            }
        }
        if (already_found) continue;
        
        // Calculate edit distance with early termination
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
    
    // Early returns for edge cases
    if (normQuery.empty() || normCandidate.empty()) {
        return FuzzyMatch(candidate, 0.0, "none");
    }
    
    // Exact match
    if (normQuery == normCandidate) {
        return FuzzyMatch(candidate, 1.0, "exact");
    }
    
    // Quick substring check first (fastest)
    if (normCandidate.find(normQuery) != std::string::npos) {
        double confidence = static_cast<double>(normQuery.length()) / normCandidate.length();
        return FuzzyMatch(candidate, std::min(0.95, confidence + 0.1), "partial");
    }
    
    if (normQuery.find(normCandidate) != std::string::npos) {
        double confidence = static_cast<double>(normCandidate.length()) / normQuery.length();
        return FuzzyMatch(candidate, std::min(0.95, confidence + 0.1), "partial");
    }
    
    // Fast common prefix/suffix check
    size_t commonPrefix = 0;
    size_t minLen = std::min(normQuery.length(), normCandidate.length());
    while (commonPrefix < minLen && normQuery[commonPrefix] == normCandidate[commonPrefix]) {
        commonPrefix++;
    }
    
    // If significant prefix match, give it a good score
    if (commonPrefix >= 3 && commonPrefix >= minLen * 0.6) {
        double confidence = static_cast<double>(commonPrefix) / std::max(normQuery.length(), normCandidate.length());
        return FuzzyMatch(candidate, std::min(0.85, confidence + 0.1), "partial");
    }
    
    // Calculate edit distance with early termination
    const int editDist = levenshteinDistance(normQuery, normCandidate);
    
    // If edit distance is too high, skip expensive calculations
    if (editDist > options.maxEditDistance) {
        return FuzzyMatch(candidate, 0.0, "none");
    }
    
    const double confidence = calculateConfidence(query, candidate, editDist);
    
    // Only check phonetic similarity if confidence is reasonable
    std::string matchType = "fuzzy";
    if (confidence > 0.3 && options.enablePhonetic && arePhoneticallySimilar(query, candidate)) {
        matchType = "phonetic";
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