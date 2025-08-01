
#ifndef VERSEFINDER_H
#define VERSEFINDER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <future>
#include <atomic>
#include "nlohmann/json.hpp"
#include "SearchCache.h"
#include "SearchOptimizer.h"
#include "PerformanceBenchmark.h"
#include "FuzzySearch.h"
#include "AutoComplete.h"
#include "SemanticSearch.h"
#include "CrossReferenceSystem.h"
#include "SearchAnalytics.h"
#include "TopicManager.h"

using json = nlohmann::json;

struct Verse {
    std::string book;
    int chapter;
    int verse;
    std::string text;
};

struct TranslationInfo {
    std::string name;
    std::string abbreviation;
    // Add other metadata if needed, like source URL, licensing info
};

class VerseFinder {
private:
    std::unordered_map<std::string, std::unordered_map<std::string, Verse>> verses;
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::string>>> keyword_index;
    std::vector<TranslationInfo> available_translations;
    std::unordered_map<std::string, std::string> book_aliases;
    std::future<void> loading_future;
    std::atomic<bool> data_loaded{false};
    std::string translations_dir;
    
    // Performance optimization components
    SearchCache search_cache;
    PerformanceBenchmark* benchmark;
    
    // Fuzzy search component
    FuzzySearch fuzzy_search;
    bool fuzzy_search_enabled = false;
    
    // Auto-complete component
    AutoComplete auto_complete;
    
    // Semantic search component
    SemanticSearch semantic_search;
    bool semantic_search_enabled = true;
    
    // Cross-reference system
    CrossReferenceSystem cross_reference_system;
    bool cross_references_enabled = true;
    
    // Search analytics
    SearchAnalytics search_analytics;
    bool analytics_enabled = true;
    
    // Topic management
    TopicManager topic_manager;
    bool topic_analysis_enabled = true;

    void loadBibleInternal(const std::string& filename);
    void loadTranslationsFromDirectory(const std::string& dir_path);
    void loadSingleTranslation(const std::string& filename);
    void loadSingleTranslationOptimized(const std::string& filename, std::mutex& data_mutex);
    std::string normalizeReference(const std::string& reference) const;
    std::string makeKey(const std::string& book, int chapter, int verse) const;
    static std::vector<std::string> tokenize(const std::string& text);
    
    // Optimized search methods
    std::vector<std::string> searchByKeywordsOptimized(const std::string& query, 
                                                      const std::string& translation) const;

public:
    VerseFinder();
    void startLoading(const std::string& filename);
    void setTranslationsDirectory(const std::string& dir_path);
    void loadAllTranslations();
    bool isReady() const;
    std::string searchByReference(const std::string& reference, const std::string& translation) const;
    std::vector<std::string> searchByChapter(const std::string& reference, const std::string& translation) const;
    std::vector<std::string> searchByKeywords(const std::string& query, const std::string& translation) const;
    std::vector<std::string> searchByFullText(const std::string& query, const std::string& translation) const;
    const std::vector<TranslationInfo>& getTranslations() const;
    void addTranslation(const std::string& json_data);
    bool saveTranslation(const std::string& json_data, const std::string& filename);
    
    // Public utility methods for UI
    bool parseReference(const std::string& reference, std::string& book, int& chapter, int& verse) const;
    std::string normalizeBookName(const std::string& book) const;
    
    // Navigation helper methods
    std::string getAdjacentVerse(const std::string& reference, const std::string& translation, int direction) const;
    bool verseExists(const std::string& book, int chapter, int verse, const std::string& translation) const;
    int getLastVerseInChapter(const std::string& book, int chapter, const std::string& translation) const;
    int getLastChapterInBook(const std::string& book, const std::string& translation) const;
    
    // Performance and caching methods
    void clearSearchCache();
    void setBenchmark(PerformanceBenchmark* bench);
    PerformanceBenchmark* getBenchmark() const;
    void printPerformanceStats() const;
    
    // Fuzzy search methods
    void enableFuzzySearch(bool enable);
    bool isFuzzySearchEnabled() const;
    void setFuzzySearchOptions(const FuzzySearchOptions& options);
    const FuzzySearchOptions& getFuzzySearchOptions() const;
    
    std::vector<std::string> searchByKeywordsFuzzy(const std::string& query, const std::string& translation) const;
    std::vector<FuzzyMatch> findBookNameSuggestions(const std::string& query) const;
    std::vector<std::string> generateQuerySuggestions(const std::string& query, const std::string& translation) const;
    
    // Auto-complete methods
    std::vector<std::string> getAutoCompletions(const std::string& input, int max_results = 10) const;
    std::vector<std::string> getSmartSuggestions(const std::string& input, int max_results = 10) const;
    void updateAutoCompleteFrequency(const std::string& query);
    void clearAutoCompleteCache();
    
    // Semantic search methods
    std::vector<std::string> searchSemantic(const std::string& query, const std::string& translation) const;
    std::vector<std::string> searchByTopic(const std::string& topic, const std::string& translation) const;
    std::vector<std::string> answerQuestion(const std::string& question, const std::string& translation) const;
    std::vector<std::string> searchBoolean(const std::string& query, const std::string& translation) const;
    std::vector<std::string> getTopicalSuggestions(const std::string& input) const;
    std::vector<std::string> getContextualSuggestions(const std::string& situation) const;
    std::vector<std::string> getRelatedTopics(const std::string& topic) const;
    QueryIntent parseNaturalLanguage(const std::string& query) const;
    void enableSemanticSearch(bool enable);
    bool isSemanticSearchEnabled() const;
    
    // Cross-reference methods
    std::vector<std::string> findCrossReferences(const std::string& verseKey) const;
    std::vector<std::string> findParallelPassages(const std::string& verseKey) const;
    std::vector<std::string> expandVerseContext(const std::string& verseKey, int contextSize = 2) const;
    void enableCrossReferences(bool enable);
    bool areCrossReferencesEnabled() const;
    
    // Analytics and discovery methods
    std::string getVerseOfTheDay() const;
    std::string getRandomVerse() const;
    std::vector<std::string> getPopularVerses(int count = 10) const;
    std::vector<std::string> getTrendingSearches(int days = 7) const;
    std::vector<std::string> getPersonalizedSuggestions() const;
    std::vector<std::string> getRecentSearches(int count = 10) const;
    
    // Bookmark and collection management
    void addToFavorites(const std::string& verseKey);
    void removeFromFavorites(const std::string& verseKey);
    std::vector<std::string> getFavoriteVerses() const;
    bool isFavorite(const std::string& verseKey) const;
    void createCollection(const std::string& name, const std::vector<std::string>& verses);
    std::vector<std::string> getCollection(const std::string& name) const;
    std::vector<std::string> getAllCollections() const;
    
    // Reading plans and guided discovery
    std::vector<std::string> generateReadingPlan(const std::string& theme) const;
    std::vector<std::string> getGuidedReadingPlan(const std::string& planType) const;
    
    // Analytics control
    void enableAnalytics(bool enable);
    bool areAnalyticsEnabled() const;
    void recordSearch(const std::string& query, const std::string& queryType, int resultCount, double executionTime);
    void recordVerseSelection(const std::string& query, const std::string& verseKey);
    
    // Topic management methods
    void enableTopicAnalysis(bool enable);
    bool isTopicAnalysisEnabled() const;
    std::vector<std::string> getVersesByTopic(const std::string& topic, int maxResults = 50) const;
    std::vector<std::string> getRelatedTopics(const std::string& topic, int maxResults = 10) const;
    std::vector<TopicSuggestion> generateTopicSuggestions(const std::string& query) const;
    std::vector<std::string> getPopularTopics(int count = 10) const;
    std::vector<std::string> getSeasonalTopicSuggestions() const;
    std::string getTopicalVerseOfTheDay(const std::string& topic = "") const;
    void addCustomTopic(const std::string& topicName, const std::vector<std::string>& keywords);
    TopicManager* getTopicManager();
};

#endif //VERSEFINDER_H
