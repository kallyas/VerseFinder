#include "SearchAnalytics.h"
#include <algorithm>
#include <random>
#include <iostream>
#include <sstream>
#include <unordered_set>

SearchAnalytics::SearchAnalytics() {
    initializeSeasonalVerses();
    initializeTopicalVerses();
}

void SearchAnalytics::initializeSeasonalVerses() {
    seasonalVerses = {
        {"christmas", {"Luke 2:11", "Isaiah 9:6", "Matthew 1:23"}},
        {"easter", {"John 11:25", "1 Corinthians 15:20", "Matthew 28:6"}},
        {"thanksgiving", {"Psalm 100:4", "1 Thessalonians 5:18", "Colossians 3:17"}},
        {"new_year", {"Jeremiah 29:11", "2 Corinthians 5:17", "Isaiah 43:19"}}
    };
}

void SearchAnalytics::initializeTopicalVerses() {
    topicalVerses = {
        {"comfort", {"Psalm 23:4", "Matthew 11:28", "2 Corinthians 1:3"}},
        {"strength", {"Philippians 4:13", "Isaiah 40:31", "Psalm 46:1"}},
        {"hope", {"Jeremiah 29:11", "Romans 15:13", "Psalm 42:11"}},
        {"love", {"1 Corinthians 13:4", "John 3:16", "1 John 4:8"}},
        {"peace", {"John 14:27", "Philippians 4:7", "Isaiah 26:3"}}
    };
}

void SearchAnalytics::recordSearch(const std::string& query, const std::string& queryType, 
                                 const std::string& translation, int resultCount, 
                                 double executionTime, bool wasSuccessful) {
    
    SearchEntry entry;
    entry.query = query;
    entry.queryType = queryType;
    entry.translation = translation;
    entry.timestamp = std::chrono::system_clock::now();
    entry.resultCount = resultCount;
    entry.executionTime = executionTime;
    entry.wasSuccessful = wasSuccessful;
    
    searchHistory.push_back(entry);
    queryFrequency[query]++;
    
    // Prune old entries if necessary
    if (searchHistory.size() > maxHistorySize) {
        pruneOldEntries();
    }
}

void SearchAnalytics::recordVerseSelection(const std::string& query, const std::string& selectedVerse) {
    // Find the most recent search entry for this query and add the selection
    for (auto it = searchHistory.rbegin(); it != searchHistory.rend(); ++it) {
        if (it->query == query) {
            it->selectedResults.push_back(selectedVerse);
            break;
        }
    }
}

void SearchAnalytics::recordVerseAccess(const std::string& verseKey, double relevanceScore) {
    auto& verse = versePopularity[verseKey];
    verse.verseKey = verseKey;
    verse.accessCount++;
    verse.lastAccessed = std::chrono::system_clock::now();
    verse.averageRelevanceScore = (verse.averageRelevanceScore + relevanceScore) / 2.0;
}

std::vector<std::string> SearchAnalytics::getMostSearchedQueries(int count) const {
    std::vector<std::pair<std::string, int>> queryPairs(queryFrequency.begin(), queryFrequency.end());
    
    std::sort(queryPairs.begin(), queryPairs.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<std::string> result;
    for (int i = 0; i < std::min(count, (int)queryPairs.size()); ++i) {
        result.push_back(queryPairs[i].first);
    }
    
    return result;
}

std::vector<std::string> SearchAnalytics::getMostPopularVerses(int count) const {
    std::vector<std::pair<std::string, int>> versePairs;
    
    for (const auto& pair : versePopularity) {
        versePairs.push_back({pair.first, pair.second.accessCount});
    }
    
    std::sort(versePairs.begin(), versePairs.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<std::string> result;
    for (int i = 0; i < std::min(count, (int)versePairs.size()); ++i) {
        result.push_back(versePairs[i].first);
    }
    
    return result;
}

std::vector<std::string> SearchAnalytics::getRecentSearches(int count) const {
    std::vector<std::string> result;
    
    int start = std::max(0, (int)searchHistory.size() - count);
    for (int i = start; i < (int)searchHistory.size(); ++i) {
        result.push_back(searchHistory[i].query);
    }
    
    std::reverse(result.begin(), result.end());
    return result;
}

std::string SearchAnalytics::getVerseOfTheDay() const {
    // Simple implementation - would be enhanced with actual verse of the day logic
    std::vector<std::string> popularVerses = {"John 3:16", "Psalm 23:1", "Romans 8:28", "Philippians 4:13"};
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, popularVerses.size() - 1);
    
    return popularVerses[dis(gen)];
}

std::string SearchAnalytics::getTopicalVerseOfTheDay(const std::string& topic) const {
    auto it = topicalVerses.find(topic);
    if (it != topicalVerses.end() && !it->second.empty()) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, it->second.size() - 1);
        return it->second[dis(gen)];
    }
    
    return getVerseOfTheDay();
}

std::string SearchAnalytics::getRandomVerse(const std::unordered_map<std::string, std::string>& allVerses) const {
    if (allVerses.empty()) return "";
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, allVerses.size() - 1);
    
    auto it = allVerses.begin();
    std::advance(it, dis(gen));
    
    return it->first + ": " + it->second;
}

void SearchAnalytics::addToFavorites(const std::string& verseKey) {
    favoriteVerses.insert(verseKey);
}

void SearchAnalytics::removeFromFavorites(const std::string& verseKey) {
    favoriteVerses.erase(verseKey);
}

std::vector<std::string> SearchAnalytics::getFavoriteVerses() const {
    return std::vector<std::string>(favoriteVerses.begin(), favoriteVerses.end());
}

bool SearchAnalytics::isFavorite(const std::string& verseKey) const {
    return favoriteVerses.find(verseKey) != favoriteVerses.end();
}

void SearchAnalytics::createCollection(const std::string& name, const std::vector<std::string>& verses) {
    customCollections[name] = verses;
}

std::vector<std::string> SearchAnalytics::getCollection(const std::string& name) const {
    auto it = customCollections.find(name);
    if (it != customCollections.end()) {
        return it->second;
    }
    return {};
}

std::vector<std::string> SearchAnalytics::getAllCollections() const {
    std::vector<std::string> result;
    for (const auto& pair : customCollections) {
        result.push_back(pair.first);
    }
    return result;
}

void SearchAnalytics::pruneOldEntries() {
    if (searchHistory.size() > maxHistorySize) {
        searchHistory.erase(searchHistory.begin(), 
                           searchHistory.begin() + (searchHistory.size() - maxHistorySize));
    }
}

std::string SearchAnalytics::categorizeQuery(const std::string& query) const {
    // Basic query categorization
    if (query.find(":") != std::string::npos) {
        return "reference";
    } else if (query.find("about") != std::string::npos || query.find("verses about") != std::string::npos) {
        return "topical";
    } else if (query.find("AND") != std::string::npos || query.find("OR") != std::string::npos) {
        return "boolean";
    }
    return "keyword";
}

int SearchAnalytics::getTotalSearches() const {
    return searchHistory.size();
}

int SearchAnalytics::getUniqueQueriesCount() const {
    return queryFrequency.size();
}

std::vector<std::string> SearchAnalytics::getRelatedQueries(const std::string& query) const {
    std::vector<std::string> related;
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    // Find queries with similar words - "People also searched for" functionality
    std::unordered_map<std::string, int> relatedQueries;
    
    for (const auto& entry : searchHistory) {
        std::string lowerHistoryQuery = entry.query;
        std::transform(lowerHistoryQuery.begin(), lowerHistoryQuery.end(), 
                      lowerHistoryQuery.begin(), ::tolower);
        
        if (lowerHistoryQuery != lowerQuery) {
            // Check for common words
            std::istringstream queryStream(lowerQuery);
            std::vector<std::string> queryWords;
            std::string word;
            while (queryStream >> word) {
                queryWords.push_back(word);
            }
            
            int commonWords = 0;
            for (const auto& queryWord : queryWords) {
                if (lowerHistoryQuery.find(queryWord) != std::string::npos) {
                    commonWords++;
                }
            }
            
            if (commonWords > 0) {
                relatedQueries[entry.query] += commonWords;
            }
        }
    }
    
    // Sort by relevance
    std::vector<std::pair<std::string, int>> sorted;
    for (const auto& entry : relatedQueries) {
        sorted.push_back({entry.first, entry.second});
    }
    
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Return top 5 related queries
    for (int i = 0; i < 5 && i < static_cast<int>(sorted.size()); ++i) {
        related.push_back(sorted[i].first);
    }
    
    return related;
}