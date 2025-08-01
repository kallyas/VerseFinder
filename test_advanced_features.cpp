#include "src/core/VerseFinder.h"
#include "src/core/TopicManager.h"
#include "src/core/SearchAnalytics.h"
#include "src/core/SemanticSearch.h"
#include <iostream>
#include <fstream>

void testAdvancedSearchFeatures() {
    std::cout << "=== Advanced Search Features Test ===" << std::endl;
    
    // Initialize VerseFinder
    VerseFinder verseFinder;
    
    // Create simple test data
    std::ofstream testBible("test_bible.json");
    testBible << R"({
        "translation": "TEST",
        "abbreviation": "TEST",
        "books": [
            {
                "name": "John",
                "chapters": [
                    {
                        "chapter": 3,
                        "verses": [
                            {
                                "verse": 16,
                                "text": "For God so loved the world that he gave his one and only Son, that whoever believes in him shall not perish but have eternal life."
                            }
                        ]
                    }
                ]
            },
            {
                "name": "Psalm",
                "chapters": [
                    {
                        "chapter": 23,
                        "verses": [
                            {
                                "verse": 1,
                                "text": "The Lord is my shepherd; I shall not want."
                            }
                        ]
                    }
                ]
            }
        ]
    })";
    testBible.close();
    
    // Load test data
    verseFinder.startLoading("test_bible.json");
    
    // Wait for loading to complete
    while (!verseFinder.isReady()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "✓ VerseFinder loaded successfully" << std::endl;
    
    // Test Topic Manager
    std::cout << "\n--- Testing Topic Manager ---" << std::endl;
    
    if (verseFinder.isTopicAnalysisEnabled()) {
        auto topicSuggestions = verseFinder.generateTopicSuggestions("love");
        std::cout << "Topic suggestions for 'love':" << std::endl;
        for (const auto& suggestion : topicSuggestions) {
            std::cout << "  - " << suggestion.topic << " (relevance: " << suggestion.relevance << ")" << std::endl;
        }
        
        auto popularTopics = verseFinder.getPopularTopics(5);
        std::cout << "Popular topics:" << std::endl;
        for (const auto& topic : popularTopics) {
            std::cout << "  - " << topic << std::endl;
        }
        
        auto seasonalTopics = verseFinder.getSeasonalTopicSuggestions();
        std::cout << "Seasonal topics:" << std::endl;
        for (const auto& topic : seasonalTopics) {
            std::cout << "  - " << topic << std::endl;
        }
    }
    
    // Test Search Analytics
    std::cout << "\n--- Testing Search Analytics ---" << std::endl;
    
    if (verseFinder.areAnalyticsEnabled()) {
        // Record some test searches
        verseFinder.recordSearch("love", "keyword", 1, 15.5);
        verseFinder.recordSearch("hope", "keyword", 3, 12.2);
        verseFinder.recordSearch("faith", "keyword", 2, 18.7);
        verseFinder.recordSearch("love hope", "keyword", 5, 22.1);
        
        auto suggestions = verseFinder.getPersonalizedSuggestions();
        std::cout << "Personalized suggestions:" << std::endl;
        for (const auto& suggestion : suggestions) {
            std::cout << "  - " << suggestion << std::endl;
        }
        
        auto recentSearches = verseFinder.getRecentSearches(3);
        std::cout << "Recent searches:" << std::endl;
        for (const auto& search : recentSearches) {
            std::cout << "  - " << search << std::endl;
        }
    }
    
    // Test Semantic Search
    std::cout << "\n--- Testing Semantic Search ---" << std::endl;
    
    SemanticSearch semanticSearch;
    
    auto queryIntent = semanticSearch.parseQuery("What does the Bible say about love?");
    std::cout << "Query: 'What does the Bible say about love?'" << std::endl;
    std::cout << "Intent type: " << queryIntent.type << std::endl;
    std::cout << "Keywords: ";
    for (const auto& keyword : queryIntent.keywords) {
        std::cout << keyword << " ";
    }
    std::cout << std::endl;
    
    // Test wildcard matching
    bool wildcardTest = semanticSearch.matchesWildcardPattern("love and hope", "love*hope");
    std::cout << "Wildcard 'love*hope' matches 'love and hope': " << (wildcardTest ? "Yes" : "No") << std::endl;
    
    // Test Boolean query parsing
    auto booleanQuery = semanticSearch.parseBooleanQuery("love AND hope NOT fear");
    std::cout << "Boolean query parsing:" << std::endl;
    std::cout << "  AND terms: ";
    for (const auto& term : booleanQuery.andTerms) {
        std::cout << term << " ";
    }
    std::cout << std::endl;
    std::cout << "  NOT terms: ";
    for (const auto& term : booleanQuery.notTerms) {
        std::cout << term << " ";
    }
    std::cout << std::endl;
    
    // Test discovery features
    std::cout << "\n--- Testing Discovery Features ---" << std::endl;
    
    auto verseOfTheDay = verseFinder.getVerseOfTheDay();
    std::cout << "Verse of the Day: " << verseOfTheDay << std::endl;
    
    auto topicalVerse = verseFinder.getTopicalVerseOfTheDay("Faith");
    std::cout << "Topical Verse of the Day (Faith): " << topicalVerse << std::endl;
    
    // Cleanup
    std::remove("test_bible.json");
    
    std::cout << "\n✓ All advanced search features tested successfully!" << std::endl;
}

int main() {
    try {
        testAdvancedSearchFeatures();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}