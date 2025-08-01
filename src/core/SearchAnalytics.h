#ifndef SEARCHANALYTICS_H
#define SEARCHANALYTICS_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <queue>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

struct SearchEntry {
    std::string query;
    std::string queryType;  // "reference", "keyword", "semantic", "boolean", etc.
    std::string translation;
    std::chrono::system_clock::time_point timestamp;
    int resultCount;
    double executionTime;  // in milliseconds
    std::vector<std::string> selectedResults;
    bool wasSuccessful;
};

struct PopularVerse {
    std::string verseKey;
    int accessCount;
    std::chrono::system_clock::time_point lastAccessed;
    double averageRelevanceScore;
};

struct SearchPattern {
    std::string pattern;
    int frequency;
    double successRate;
    std::vector<std::string> commonQueries;
};

struct VerseOfTheDayEntry {
    std::string verseKey;
    std::string source;  // "random", "seasonal", "topical", "popular"
    std::chrono::system_clock::time_point date;
    std::string theme;
};

class SearchAnalytics {
private:
    // Search history and patterns
    std::vector<SearchEntry> searchHistory;
    std::unordered_map<std::string, int> queryFrequency;
    std::unordered_map<std::string, SearchPattern> searchPatterns;
    
    // Verse popularity and access patterns
    std::unordered_map<std::string, PopularVerse> versePopularity;
    std::unordered_map<std::string, int> topicSearchCount;
    
    // Performance metrics
    std::unordered_map<std::string, std::vector<double>> performanceMetrics;
    std::unordered_map<std::string, double> averageExecutionTimes;
    
    // Favorites and bookmarks
    std::unordered_set<std::string> favoriteVerses;
    std::unordered_map<std::string, std::vector<std::string>> customCollections;
    
    // Verse of the day system
    std::vector<VerseOfTheDayEntry> verseOfTheDayHistory;
    std::unordered_map<std::string, std::vector<std::string>> seasonalVerses;
    std::unordered_map<std::string, std::vector<std::string>> topicalVerses;
    
    // Configuration
    size_t maxHistorySize = 10000;
    int trendsAnalysisDays = 30;
    
    // Helper methods
    void pruneOldEntries();
    std::string categorizeQuery(const std::string& query) const;
    void updateSearchPatterns(const SearchEntry& entry);
    void initializeSeasonalVerses();
    void initializeTopicalVerses();
    
public:
    SearchAnalytics();
    
    // Search tracking
    void recordSearch(const std::string& query, const std::string& queryType, 
                     const std::string& translation, int resultCount, 
                     double executionTime, bool wasSuccessful);
    void recordVerseSelection(const std::string& query, const std::string& selectedVerse);
    void recordVerseAccess(const std::string& verseKey, double relevanceScore = 1.0);
    
    // Analytics queries
    std::vector<std::string> getMostSearchedQueries(int count = 10) const;
    std::vector<std::string> getMostPopularVerses(int count = 10) const;
    std::vector<std::string> getTrendingQueries(int days = 7) const;
    std::vector<std::string> getRecentSearches(int count = 20) const;
    
    // Performance analysis
    std::unordered_map<std::string, double> getAverageSearchTimes() const;
    std::vector<std::pair<std::string, double>> getSearchSuccessRates() const;
    std::unordered_map<std::string, int> getQueryTypeDistribution() const;
    
    // Search suggestions based on history
    std::vector<std::string> getSuggestionsBasedOnHistory(const std::string& partialQuery) const;
    std::vector<std::string> getRelatedQueries(const std::string& query) const;
    std::vector<std::string> getPersonalizedSuggestions() const;
    
    // Favorites and collections
    void addToFavorites(const std::string& verseKey);
    void removeFromFavorites(const std::string& verseKey);
    std::vector<std::string> getFavoriteVerses() const;
    bool isFavorite(const std::string& verseKey) const;
    
    void createCollection(const std::string& name, const std::vector<std::string>& verses);
    void addToCollection(const std::string& collectionName, const std::string& verseKey);
    void removeFromCollection(const std::string& collectionName, const std::string& verseKey);
    std::vector<std::string> getCollection(const std::string& name) const;
    std::vector<std::string> getAllCollections() const;
    void deleteCollection(const std::string& name);
    
    // Verse of the Day system
    std::string getVerseOfTheDay() const;
    std::string getTopicalVerseOfTheDay(const std::string& topic) const;
    std::string getSeasonalVerseOfTheDay() const;
    std::string getRandomVerse(const std::unordered_map<std::string, std::string>& allVerses) const;
    void recordVerseOfTheDay(const std::string& verseKey, const std::string& source, const std::string& theme = "");
    
    // Reading plans
    std::vector<std::string> generateWeeklyReadingPlan(const std::string& theme) const;
    std::vector<std::string> getGuidedReadingPlan(const std::string& planType) const;
    std::vector<std::string> getPersonalizedReadingPlan() const;
    
    // A/B testing framework
    void recordABTestResult(const std::string& testName, const std::string& variant, 
                           const std::string& metric, double value);
    std::unordered_map<std::string, double> getABTestResults(const std::string& testName) const;
    
    // Usage patterns and insights
    std::vector<std::pair<int, int>> getUsagePatternsByHour() const;  // hour -> search count
    std::vector<std::pair<std::string, int>> getUsagePatternsByDay() const;  // day of week -> search count
    std::unordered_map<std::string, int> getTopicInterests() const;
    
    // Data management
    void clearHistory();
    void clearFavorites();
    void exportData() const;
    void importData(const std::string& jsonData);
    void setMaxHistorySize(size_t size);
    
    // Statistics
    int getTotalSearches() const;
    int getUniqueQueriesCount() const;
    double getAverageResultsPerSearch() const;
    std::chrono::system_clock::time_point getFirstSearchDate() const;
    std::chrono::system_clock::time_point getLastSearchDate() const;
};

#endif // SEARCHANALYTICS_H