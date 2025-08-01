#include "AutoComplete.h"
#include "VerseFinder.h"
#include <regex>
#include <sstream>
#include <cmath>

AutoComplete::AutoComplete() : root(std::make_unique<TrieNode>()) {
}

void AutoComplete::insertWord(const std::string& word) {
    if (word.empty()) return;
    
    TrieNode* current = root.get();
    std::string lower_word = word;
    std::transform(lower_word.begin(), lower_word.end(), lower_word.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    for (char ch : lower_word) {
        if (current->children.find(ch) == current->children.end()) {
            current->children[ch] = std::make_unique<TrieNode>();
        }
        current = current->children[ch].get();
        current->addCompletion(word);
    }
    current->is_end_of_word = true;
}

void AutoComplete::buildIndex(const std::unordered_map<std::string, std::unordered_map<std::string, Verse>>& verses) {
    clear();
    
    for (const auto& translation_pair : verses) {
        for (const auto& verse_pair : translation_pair.second) {
            const Verse& verse = verse_pair.second;
            
            // Add book name to reference patterns
            addBookName(verse.book);
            
            // Add reference pattern
            addReferencePattern(verse.book, verse.chapter, verse.verse);
            
            // Add verse text for keyword completions
            addVerseText(verse.text);
        }
    }
}

void AutoComplete::addBookName(const std::string& book_name) {
    insertWord(book_name);
    
    // Add to reference patterns for chapter/verse completion
    reference_patterns[book_name] = std::set<std::string>();
}

void AutoComplete::addVerseText(const std::string& text) {
    // Simple tokenization - split on whitespace and punctuation
    std::regex word_regex(R"(\b\w+\b)");
    std::sregex_iterator words_begin(text.begin(), text.end(), word_regex);
    std::sregex_iterator words_end;
    
    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::string word = (*i).str();
        if (word.length() >= 3) { // Only index words of 3+ characters
            insertWord(word);
            updateWordFrequency(word);
        }
    }
}

void AutoComplete::addReferencePattern(const std::string& book, int chapter, int verse) {
    // Add patterns like "John 3", "John 3:16"
    std::string chapter_ref = book + " " + std::to_string(chapter);
    std::string verse_ref = chapter_ref + ":" + std::to_string(verse);
    
    reference_patterns[book].insert(chapter_ref);
    reference_patterns[book].insert(verse_ref);
    
    insertWord(chapter_ref);
    insertWord(verse_ref);
}

bool AutoComplete::isReferencePattern(const std::string& input) const {
    // Check if input looks like a Bible reference
    std::regex reference_regex(R"(\b\w+\s*\d+(?::\d+)?\s*$)");
    return std::regex_search(input, reference_regex);
}

std::vector<std::string> AutoComplete::generateReferenceCompletions(const std::string& input) const {
    std::vector<std::string> completions;
    
    // Extract book name from input
    std::regex book_regex(R"(^(\w+(?:\s+\w+)*))");
    std::smatch book_match;
    
    if (std::regex_search(input, book_match, book_regex)) {
        std::string book_part = book_match[1].str();
        
        // Find matching books
        for (const auto& pattern_pair : reference_patterns) {
            const std::string& book = pattern_pair.first;
            if (book.find(book_part) == 0 || 
                (book.length() >= book_part.length() && 
                 std::equal(book_part.begin(), book_part.end(), book.begin(),
                           [](char a, char b) { return std::tolower(a) == std::tolower(b); }))) {
                
                // Add matching patterns for this book
                for (const std::string& pattern : pattern_pair.second) {
                    if (pattern.find(input) == 0) {
                        completions.push_back(pattern);
                        if (completions.size() >= 10) break;
                    }
                }
            }
        }
    }
    
    return completions;
}

std::vector<std::string> AutoComplete::generateKeywordCompletions(const std::string& input) const {
    std::vector<std::string> completions;
    
    if (input.empty()) return completions;
    
    std::string lower_input = input;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    TrieNode* current = root.get();
    
    // Navigate to the prefix node
    for (char ch : lower_input) {
        if (current->children.find(ch) == current->children.end()) {
            return completions; // No completions found
        }
        current = current->children[ch].get();
    }
    
    // Collect completions from this node
    collectCompletions(current, input, completions, 10);
    
    return completions;
}

void AutoComplete::collectCompletions(TrieNode* node, const std::string& /* prefix */, 
                                    std::vector<std::string>& results, int max_results) const {
    if (!node || results.size() >= static_cast<size_t>(max_results)) return;
    
    // Add all completions stored at this node
    for (const std::string& completion : node->completions) {
        if (results.size() >= static_cast<size_t>(max_results)) break;
        results.push_back(completion);
    }
}

std::vector<std::string> AutoComplete::getCompletions(const std::string& input, int max_results) const {
    if (input.empty()) return {};
    
    // Check cache first
    auto cache_it = suggestion_cache.find(input);
    if (cache_it != suggestion_cache.end()) {
        auto result = cache_it->second;
        if (result.size() > static_cast<size_t>(max_results)) {
            result.resize(max_results);
        }
        return result;
    }
    
    std::vector<std::string> completions;
    
    if (isReferencePattern(input)) {
        completions = generateReferenceCompletions(input);
    } else {
        completions = generateKeywordCompletions(input);
    }
    
    // Rank and limit results
    rankSuggestions(completions, input);
    if (completions.size() > static_cast<size_t>(max_results)) {
        completions.resize(max_results);
    }
    
    // Cache the result
    if (suggestion_cache.size() >= MAX_CACHE_SIZE) {
        manageCacheSize();
    }
    suggestion_cache[input] = completions;
    
    return completions;
}

std::vector<std::string> AutoComplete::getSmartSuggestions(const std::string& input, int max_results) const {
    std::vector<std::string> suggestions = getCompletions(input, max_results * 2);
    
    // Apply additional smart filtering and ranking
    rankSuggestions(suggestions, input);
    
    if (suggestions.size() > static_cast<size_t>(max_results)) {
        suggestions.resize(max_results);
    }
    
    return suggestions;
}

double AutoComplete::calculateWordScore(const std::string& word, const std::string& input) const {
    double score = 0.0;
    
    // Exact prefix match gets highest score
    if (word.find(input) == 0) {
        score += 100.0;
    }
    
    // Case-insensitive prefix match
    std::string lower_word = word;
    std::string lower_input = input;
    std::transform(lower_word.begin(), lower_word.end(), lower_word.begin(), ::tolower);
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);
    
    if (lower_word.find(lower_input) == 0) {
        score += 50.0;
    }
    
    // Frequency boost
    auto freq_it = word_frequency.find(word);
    if (freq_it != word_frequency.end()) {
        score += std::log(freq_it->second + 1) * 10.0;
    }
    
    // Length penalty (prefer shorter completions)
    score -= word.length() * 0.1;
    
    return score;
}

void AutoComplete::rankSuggestions(std::vector<std::string>& suggestions, const std::string& input) const {
    std::sort(suggestions.begin(), suggestions.end(),
              [this, &input](const std::string& a, const std::string& b) {
                  return calculateWordScore(a, input) > calculateWordScore(b, input);
              });
}

void AutoComplete::updateWordFrequency(const std::string& word) {
    word_frequency[word]++;
}

void AutoComplete::clearCache() {
    suggestion_cache.clear();
}

void AutoComplete::manageCacheSize() const {
    // Remove oldest half of cache entries when limit is reached
    auto it = suggestion_cache.begin();
    std::advance(it, suggestion_cache.size() / 2);
    suggestion_cache.erase(suggestion_cache.begin(), it);
}

size_t AutoComplete::getMemoryUsage() const {
    // Rough estimation of memory usage
    size_t size = 0;
    
    // Trie structure (approximation)
    size += sizeof(TrieNode) * 1000; // Rough estimate
    
    // Reference patterns
    for (const auto& pattern_pair : reference_patterns) {
        size += pattern_pair.first.size();
        for (const auto& pattern : pattern_pair.second) {
            size += pattern.size();
        }
    }
    
    // Word frequency map
    for (const auto& freq_pair : word_frequency) {
        size += freq_pair.first.size() + sizeof(int);
    }
    
    // Cache
    for (const auto& cache_pair : suggestion_cache) {
        size += cache_pair.first.size();
        for (const auto& suggestion : cache_pair.second) {
            size += suggestion.size();
        }
    }
    
    return size;
}

void AutoComplete::clear() {
    root = std::make_unique<TrieNode>();
    reference_patterns.clear();
    word_frequency.clear();
    clearCache();
}