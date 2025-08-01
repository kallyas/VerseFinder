#ifndef TOPICMANAGER_H
#define TOPICMANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// Forward declaration to avoid circular dependency
struct Verse;

struct TopicCluster {
    std::string name;
    std::vector<std::string> keywords;
    std::vector<std::string> relatedTopics;
    std::unordered_set<std::string> verseKeys;
    double coherenceScore;
    int searchFrequency;
};

struct VerseTopicScore {
    std::string verseKey;
    std::string topic;
    double relevanceScore;
    std::vector<std::string> matchedKeywords;
};

struct TopicSuggestion {
    std::string topic;
    double relevance;
    std::string reason;
    std::vector<std::string> sampleVerses;
};

class TopicManager {
private:
    // Core topic data
    std::unordered_map<std::string, TopicCluster> topics;
    std::unordered_map<std::string, std::vector<std::string>> verseTopicMapping;
    std::unordered_map<std::string, int> topicPopularity;
    
    // Seasonal and liturgical topics
    std::unordered_map<std::string, std::vector<std::string>> seasonalTopics;
    std::unordered_map<std::string, std::vector<std::string>> liturgicalTopics;
    
    // Topic relationships
    std::unordered_map<std::string, std::vector<std::string>> topicHierarchy;
    std::unordered_map<std::string, double> topicSimilarity;
    
    // Search analytics
    std::unordered_map<std::string, int> topicSearchCount;
    std::vector<std::pair<std::string, std::string>> searchHistory;
    
    // Initialize predefined topics
    void initializeCoreTopics();
    void initializeSeasonalTopics();
    void initializeLiturgicalTopics();
    void buildTopicHierarchy();
    
    // Topic analysis helpers
    double calculateTopicCoherence(const TopicCluster& cluster, 
                                 const std::unordered_map<std::string, std::unordered_map<std::string, Verse>>& verses) const;
    std::vector<std::string> extractTopicKeywords(const std::vector<std::string>& verseTexts) const;
    double calculateSemanticSimilarity(const std::string& topic1, const std::string& topic2) const;
    
    // Machine learning helpers (basic implementations)
    std::vector<TopicCluster> performTopicClustering(const std::vector<std::string>& documents) const;
    std::unordered_map<std::string, double> calculateTFIDF(const std::vector<std::string>& documents) const;
    
public:
    TopicManager();
    
    // Topic organization and management
    void buildTopicIndex(const std::unordered_map<std::string, std::unordered_map<std::string, Verse>>& verses);
    void addCustomTopic(const std::string& topicName, const std::vector<std::string>& keywords);
    void updateTopicKeywords(const std::string& topicName, const std::vector<std::string>& newKeywords);
    void removeTopicFromVerse(const std::string& verseKey, const std::string& topic);
    
    // Topic discovery and analysis
    std::vector<VerseTopicScore> analyzeVerseTopics(const std::string& verseText, const std::string& verseKey) const;
    std::vector<std::string> getVersesByTopic(const std::string& topic, int maxResults = 50) const;
    std::vector<std::string> getRelatedTopics(const std::string& topic, int maxResults = 10) const;
    std::vector<TopicCluster> getTopicClusters() const;
    
    // Topic-based search
    std::vector<std::string> searchByTopicHierarchy(const std::string& parentTopic) const;
    std::vector<std::string> findSimilarTopics(const std::string& topic, double threshold = 0.7) const;
    std::vector<std::string> getTopicIntersection(const std::vector<std::string>& topics) const;
    
    // Suggestions and recommendations
    std::vector<TopicSuggestion> generateTopicSuggestions(const std::string& query) const;
    std::vector<std::string> getPopularTopics(int count = 10) const;
    std::vector<std::string> getTrendingTopics(int days = 30) const;
    std::vector<std::string> getSeasonalSuggestions() const;
    std::vector<std::string> getLiturgicalSuggestions(const std::string& season = "") const;
    
    // Collection management
    void createTopicCollection(const std::string& collectionName, const std::vector<std::string>& topics);
    std::vector<std::string> getTopicCollection(const std::string& collectionName) const;
    std::vector<std::string> getAllCollections() const;
    void deleteTopicCollection(const std::string& collectionName);
    
    // Analytics and learning
    void recordTopicSearch(const std::string& topic);
    void recordTopicSelection(const std::string& query, const std::string& selectedTopic);
    std::vector<std::pair<std::string, int>> getTopicSearchStats() const;
    void updateTopicRelevance(const std::string& topic, const std::vector<std::string>& positiveVerses, 
                            const std::vector<std::string>& negativeVerses);
    
    // Import/Export
    void loadTopicConfiguration(const std::string& configPath);
    void saveTopicConfiguration(const std::string& configPath) const;
    std::string exportTopicsAsJson() const;
    void importTopicsFromJson(const std::string& jsonData);
    
    // Verse of the Day functionality
    std::string getVerseOfTheDay(const std::string& source = "general") const;
    std::string getTopicalVerseOfTheDay(const std::string& topic) const;
    std::vector<std::string> getWeeklyReadingPlan(const std::string& theme) const;
    
    // Topic browsing interface helpers
    std::vector<std::string> getTopicTree() const;
    std::vector<std::string> getChildTopics(const std::string& parentTopic) const;
    std::string getParentTopic(const std::string& childTopic) const;
    bool isTopicLeaf(const std::string& topic) const;
    
    // Statistics
    int getTopicCount() const;
    int getVerseCountForTopic(const std::string& topic) const;
    double getTopicCoverageRatio() const;
    std::unordered_map<std::string, int> getTopicDistribution() const;
    
    // Configuration
    void setTopicSimilarityThreshold(double threshold);
    void setMaxTopicsPerVerse(int maxTopics);
    void enableSeasonalSuggestions(bool enabled);
    void enableLiturgicalSuggestions(bool enabled);
};

#endif // TOPICMANAGER_H