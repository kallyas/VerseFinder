#ifndef SEMANTICSEARCH_H
#define SEMANTICSEARCH_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <regex>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

struct QueryIntent {
    enum Type {
        REFERENCE_LOOKUP,      // "John 3:16"
        KEYWORD_SEARCH,        // "love joy peace"
        TOPICAL_SEARCH,        // "verses about hope"
        QUESTION_BASED,        // "What does the Bible say about forgiveness?"
        CONTEXTUAL_REQUEST,    // "verses for difficult times"
        BOOLEAN_SEARCH,        // "love AND hope NOT fear"
        SEMANTIC_SEARCH        // "comfort in suffering"
    };
    
    Type type;
    std::string originalQuery;
    std::vector<std::string> keywords;
    std::vector<std::string> topics;
    std::string subject;
    double confidence;
};

struct TopicScore {
    std::string topic;
    double relevance;
    std::vector<std::string> relatedWords;
};

struct SemanticMatch {
    std::string verseKey;
    std::string text;
    double semanticScore;
    std::vector<std::string> matchedTopics;
    std::vector<std::string> matchedKeywords;
};

class SemanticSearch {
private:
    // Topic-to-keywords mapping for basic semantic understanding
    std::unordered_map<std::string, std::vector<std::string>> topicKeywords;
    
    // Common question patterns and their mapping to topics
    std::unordered_map<std::string, std::string> questionPatterns;
    
    // Contextual situation mapping
    std::unordered_map<std::string, std::vector<std::string>> contextualSituations;
    
    // Boolean operator patterns
    std::vector<std::regex> booleanPatterns;
    
    // Natural language processing helpers
    std::unordered_set<std::string> stopWords;
    std::unordered_map<std::string, std::vector<std::string>> synonyms;
    
    // Topic clustering data
    std::unordered_map<std::string, std::vector<std::string>> topicClusters;
    
    // Initialize semantic knowledge base
    void initializeTopicKeywords();
    void initializeQuestionPatterns();
    void initializeContextualSituations();
    void initializeBooleanPatterns();
    void initializeStopWords();
    void initializeSynonyms();
    
    // Query processing helpers
    std::vector<std::string> tokenizeAndFilter(const std::string& query) const;
    std::string normalizeQuery(const std::string& query) const;
    std::vector<std::string> extractKeywords(const std::string& query) const;
    std::vector<std::string> expandWithSynonyms(const std::vector<std::string>& keywords) const;
    
    // Intent recognition
    QueryIntent::Type detectQueryType(const std::string& query) const;
    std::vector<std::string> extractTopicsFromQuery(const std::string& query) const;
    std::string extractSubjectFromQuestion(const std::string& query) const;
    
    // Semantic scoring
    double calculateSemanticScore(const std::string& verseText, const QueryIntent& intent) const;
    std::vector<TopicScore> getTopicRelevance(const std::string& text) const;

public:
    SemanticSearch();
    
    // Main semantic search interface
    QueryIntent parseQuery(const std::string& query) const;
    std::vector<std::string> generateSemanticKeywords(const QueryIntent& intent) const;
    std::vector<std::string> getRelatedTopics(const std::string& topic) const;
    
    // Topic-based search
    std::vector<std::string> searchByTopic(const std::string& topic) const;
    std::vector<TopicScore> analyzeTopicRelevance(const std::string& text) const;
    
    // Question answering
    std::vector<std::string> answerQuestion(const std::string& question) const;
    std::string extractQuestionTopic(const std::string& question) const;
    
    // Boolean search processing
    struct BooleanQuery {
        std::vector<std::string> andTerms;
        std::vector<std::string> orTerms;
        std::vector<std::string> notTerms;
    };
    
    BooleanQuery parseBooleanQuery(const std::string& query) const;
    
    // Suggestion generation
    std::vector<std::string> generateTopicalSuggestions(const std::string& input) const;
    std::vector<std::string> generateContextualSuggestions(const std::string& situation) const;
    
    // Learning and adaptation
    void updateTopicRelevance(const std::string& query, const std::vector<std::string>& selectedVerses);
    void addCustomTopic(const std::string& topic, const std::vector<std::string>& keywords);
    
    // Configuration
    void loadSemanticConfig(const std::string& configJson);
    std::string exportSemanticConfig() const;
    
    // Analytics
    std::vector<std::string> getMostSearchedTopics() const;
    std::unordered_map<std::string, int> getTopicSearchFrequency() const;
};

#endif // SEMANTICSEARCH_H