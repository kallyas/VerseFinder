#include "TopicManager.h"
#include <algorithm>
#include <sstream>
#include <random>
#include <chrono>
#include <cmath>

// Include Verse struct definition
struct Verse {
    std::string book;
    int chapter;
    int verse;
    std::string text;
};

TopicManager::TopicManager() {
    initializeCoreTopics();
    initializeSeasonalTopics();
    initializeLiturgicalTopics();
    buildTopicHierarchy();
}

void TopicManager::initializeCoreTopics() {
    // Core biblical themes with keywords
    topics["Faith"] = TopicCluster{
        "Faith",
        {"faith", "believe", "trust", "confidence", "assurance", "conviction"},
        {"Hope", "Prayer", "Salvation"},
        {},
        0.9,
        0
    };
    
    topics["Hope"] = TopicCluster{
        "Hope",
        {"hope", "expectation", "future", "promise", "anticipation", "waiting"},
        {"Faith", "Joy", "Encouragement"},
        {},
        0.9,
        0
    };
    
    topics["Love"] = TopicCluster{
        "Love",
        {"love", "beloved", "charity", "affection", "compassion", "kindness"},
        {"Forgiveness", "Grace", "Mercy"},
        {},
        0.95,
        0
    };
    
    topics["Forgiveness"] = TopicCluster{
        "Forgiveness",
        {"forgive", "pardon", "mercy", "grace", "reconciliation", "redemption"},
        {"Love", "Mercy", "Repentance"},
        {},
        0.9,
        0
    };
    
    topics["Prayer"] = TopicCluster{
        "Prayer",
        {"pray", "prayer", "petition", "intercession", "supplication", "request"},
        {"Faith", "Worship", "Communion"},
        {},
        0.85,
        0
    };
    
    topics["Salvation"] = TopicCluster{
        "Salvation",
        {"salvation", "saved", "redemption", "deliverance", "rescue", "eternal life"},
        {"Faith", "Grace", "Forgiveness"},
        {},
        0.95,
        0
    };
    
    topics["Wisdom"] = TopicCluster{
        "Wisdom",
        {"wisdom", "wise", "understanding", "knowledge", "discernment", "prudence"},
        {"Truth", "Learning", "Guidance"},
        {},
        0.9,
        0
    };
    
    topics["Peace"] = TopicCluster{
        "Peace",
        {"peace", "peaceful", "calm", "tranquility", "rest", "stillness"},
        {"Joy", "Comfort", "Rest"},
        {},
        0.9,
        0
    };
    
    topics["Joy"] = TopicCluster{
        "Joy",
        {"joy", "joyful", "rejoice", "gladness", "happiness", "delight"},
        {"Peace", "Hope", "Celebration"},
        {},
        0.9,
        0
    };
    
    topics["Strength"] = TopicCluster{
        "Strength",
        {"strength", "strong", "power", "mighty", "courage", "boldness"},
        {"Faith", "Victory", "Endurance"},
        {},
        0.85,
        0
    };
}

void TopicManager::initializeSeasonalTopics() {
    seasonalTopics["Christmas"] = {"Incarnation", "Birth", "Emmanuel", "Nativity", "Joy"};
    seasonalTopics["Easter"] = {"Resurrection", "Victory", "Life", "Hope", "Salvation"};
    seasonalTopics["Thanksgiving"] = {"Gratitude", "Blessing", "Provision", "Harvest"};
    seasonalTopics["New Year"] = {"New Beginning", "Hope", "Renewal", "Purpose"};
    seasonalTopics["Spring"] = {"New Life", "Renewal", "Growth", "Creation"};
    seasonalTopics["Summer"] = {"Rest", "Vacation", "Provision", "Family"};
    seasonalTopics["Fall"] = {"Harvest", "Gratitude", "Preparation", "Wisdom"};
    seasonalTopics["Winter"] = {"Comfort", "Warmth", "Hope", "Endurance"};
}

void TopicManager::initializeLiturgicalTopics() {
    liturgicalTopics["Advent"] = {"Hope", "Waiting", "Preparation", "Prophecy"};
    liturgicalTopics["Lent"] = {"Repentance", "Fasting", "Prayer", "Sacrifice"};
    liturgicalTopics["Pentecost"] = {"Holy Spirit", "Power", "Gifts", "Church"};
    liturgicalTopics["Ordinary Time"] = {"Growth", "Discipleship", "Service", "Faith"};
}

void TopicManager::buildTopicHierarchy() {
    // Build simple hierarchical relationships
    topicHierarchy["Theological Concepts"] = {"Faith", "Hope", "Love", "Salvation", "Grace"};
    topicHierarchy["Spiritual Practices"] = {"Prayer", "Worship", "Fasting", "Meditation"};
    topicHierarchy["Character Qualities"] = {"Wisdom", "Patience", "Kindness", "Humility"};
    topicHierarchy["Relationships"] = {"Love", "Forgiveness", "Marriage", "Family"};
    topicHierarchy["Emotions"] = {"Joy", "Peace", "Fear", "Anger", "Sadness"};
}

void TopicManager::buildTopicIndex(const std::unordered_map<std::string, std::unordered_map<std::string, Verse>>& verses) {
    // Build topic index by analyzing verse content
    for (const auto& translation : verses) {
        for (const auto& verse : translation.second) {
            auto topicScores = analyzeVerseTopics(verse.second.text, verse.first);
            for (const auto& score : topicScores) {
                topics[score.topic].verseKeys.insert(score.verseKey);
                verseTopicMapping[score.verseKey].push_back(score.topic);
            }
        }
    }
}

std::vector<VerseTopicScore> TopicManager::analyzeVerseTopics(const std::string& verseText, const std::string& verseKey) const {
    std::vector<VerseTopicScore> results;
    
    // Convert text to lowercase for matching
    std::string lowerText = verseText;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
    
    for (const auto& topic : topics) {
        double score = 0.0;
        std::vector<std::string> matchedKeywords;
        
        // Check for keyword matches
        for (const auto& keyword : topic.second.keywords) {
            if (lowerText.find(keyword) != std::string::npos) {
                score += 1.0;
                matchedKeywords.push_back(keyword);
            }
        }
        
        // Normalize score based on number of keywords
        if (!topic.second.keywords.empty()) {
            score /= topic.second.keywords.size();
        }
        
        // Only include if there's a meaningful match
        if (score > 0.1) {
            results.push_back({verseKey, topic.first, score, matchedKeywords});
        }
    }
    
    // Sort by relevance score
    std::sort(results.begin(), results.end(), 
              [](const VerseTopicScore& a, const VerseTopicScore& b) {
                  return a.relevanceScore > b.relevanceScore;
              });
    
    return results;
}

std::vector<std::string> TopicManager::getVersesByTopic(const std::string& topic, int maxResults) const {
    std::vector<std::string> results;
    
    auto it = topics.find(topic);
    if (it != topics.end()) {
        for (const auto& verseKey : it->second.verseKeys) {
            results.push_back(verseKey);
            if (results.size() >= static_cast<size_t>(maxResults)) {
                break;
            }
        }
    }
    
    return results;
}

std::vector<std::string> TopicManager::getRelatedTopics(const std::string& topic, int maxResults) const {
    std::vector<std::string> results;
    
    auto it = topics.find(topic);
    if (it != topics.end()) {
        for (const auto& relatedTopic : it->second.relatedTopics) {
            results.push_back(relatedTopic);
            if (results.size() >= static_cast<size_t>(maxResults)) {
                break;
            }
        }
    }
    
    return results;
}

std::vector<TopicSuggestion> TopicManager::generateTopicSuggestions(const std::string& query) const {
    std::vector<TopicSuggestion> suggestions;
    
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    for (const auto& topic : topics) {
        double relevance = 0.0;
        
        // Check if query matches topic name
        std::string lowerTopicName = topic.first;
        std::transform(lowerTopicName.begin(), lowerTopicName.end(), lowerTopicName.begin(), ::tolower);
        
        if (lowerTopicName.find(lowerQuery) != std::string::npos) {
            relevance += 0.8;
        }
        
        // Check if query matches keywords
        for (const auto& keyword : topic.second.keywords) {
            if (lowerQuery.find(keyword) != std::string::npos) {
                relevance += 0.3;
            }
        }
        
        if (relevance > 0.0) {
            // Get sample verses
            std::vector<std::string> sampleVerses;
            int count = 0;
            for (const auto& verseKey : topic.second.verseKeys) {
                sampleVerses.push_back(verseKey);
                if (++count >= 3) break;
            }
            
            suggestions.push_back({
                topic.first,
                relevance,
                "Contains keywords related to your search",
                sampleVerses
            });
        }
    }
    
    // Sort by relevance
    std::sort(suggestions.begin(), suggestions.end(),
              [](const TopicSuggestion& a, const TopicSuggestion& b) {
                  return a.relevance > b.relevance;
              });
    
    return suggestions;
}

std::vector<std::string> TopicManager::getPopularTopics(int count) const {
    std::vector<std::pair<std::string, int>> topicCounts;
    
    for (const auto& topic : topicPopularity) {
        topicCounts.push_back({topic.first, topic.second});
    }
    
    std::sort(topicCounts.begin(), topicCounts.end(),
              [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                  return a.second > b.second;
              });
    
    std::vector<std::string> results;
    for (int i = 0; i < count && i < static_cast<int>(topicCounts.size()); ++i) {
        results.push_back(topicCounts[i].first);
    }
    
    return results;
}

std::vector<std::string> TopicManager::getSeasonalSuggestions() const {
    // Get current month to determine season
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    int month = tm.tm_mon + 1; // tm_mon is 0-based
    
    std::string season;
    if (month >= 12 || month <= 2) {
        season = "Winter";
    } else if (month >= 3 && month <= 5) {
        season = "Spring";
    } else if (month >= 6 && month <= 8) {
        season = "Summer";
    } else {
        season = "Fall";
    }
    
    auto it = seasonalTopics.find(season);
    if (it != seasonalTopics.end()) {
        return it->second;
    }
    
    return {};
}

void TopicManager::recordTopicSearch(const std::string& topic) {
    topicSearchCount[topic]++;
    topicPopularity[topic]++;
}

void TopicManager::addCustomTopic(const std::string& topicName, const std::vector<std::string>& keywords) {
    TopicCluster cluster{
        topicName,
        keywords,
        {},
        {},
        0.8,
        0
    };
    
    topics[topicName] = cluster;
}

std::string TopicManager::getVerseOfTheDay(const std::string& source) const {
    if (source == "seasonal") {
        auto seasonalTopics = getSeasonalSuggestions();
        if (!seasonalTopics.empty()) {
            auto topicVerses = getVersesByTopic(seasonalTopics[0], 10);
            if (!topicVerses.empty()) {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(0, topicVerses.size() - 1);
                return topicVerses[dis(gen)];
            }
        }
    }
    
    // Default to popular verses
    auto popularTopics = getPopularTopics(5);
    if (!popularTopics.empty()) {
        auto topicVerses = getVersesByTopic(popularTopics[0], 10);
        if (!topicVerses.empty()) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, topicVerses.size() - 1);
            return topicVerses[dis(gen)];
        }
    }
    
    return "John 3:16"; // Fallback
}

std::string TopicManager::getTopicalVerseOfTheDay(const std::string& topic) const {
    if (!topic.empty()) {
        auto topicVerses = getVersesByTopic(topic, 10);
        if (!topicVerses.empty()) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, topicVerses.size() - 1);
            return topicVerses[dis(gen)];
        }
    }
    
    return getVerseOfTheDay("seasonal");
}

std::vector<std::string> TopicManager::getTopicTree() const {
    std::vector<std::string> tree;
    for (const auto& parent : topicHierarchy) {
        tree.push_back(parent.first);
        for (const auto& child : parent.second) {
            tree.push_back("  " + child);
        }
    }
    return tree;
}

int TopicManager::getTopicCount() const {
    return topics.size();
}

int TopicManager::getVerseCountForTopic(const std::string& topic) const {
    auto it = topics.find(topic);
    if (it != topics.end()) {
        return it->second.verseKeys.size();
    }
    return 0;
}

std::vector<TopicCluster> TopicManager::getTopicClusters() const {
    std::vector<TopicCluster> clusters;
    for (const auto& topic : topics) {
        clusters.push_back(topic.second);
    }
    return clusters;
}

std::string TopicManager::exportTopicsAsJson() const {
    json topicsJson;
    
    for (const auto& topic : topics) {
        json topicJson;
        topicJson["name"] = topic.second.name;
        topicJson["keywords"] = topic.second.keywords;
        topicJson["relatedTopics"] = topic.second.relatedTopics;
        topicJson["coherenceScore"] = topic.second.coherenceScore;
        topicJson["searchFrequency"] = topic.second.searchFrequency;
        
        // Convert unordered_set to vector for JSON serialization
        std::vector<std::string> verseKeysVector(topic.second.verseKeys.begin(), topic.second.verseKeys.end());
        topicJson["verseKeys"] = verseKeysVector;
        
        topicsJson[topic.first] = topicJson;
    }
    
    return topicsJson.dump(2);
}

void TopicManager::importTopicsFromJson(const std::string& jsonData) {
    try {
        json topicsJson = json::parse(jsonData);
        
        for (auto& [topicName, topicData] : topicsJson.items()) {
            TopicCluster cluster;
            cluster.name = topicData.value("name", topicName);
            cluster.keywords = topicData.value("keywords", std::vector<std::string>{});
            cluster.relatedTopics = topicData.value("relatedTopics", std::vector<std::string>{});
            cluster.coherenceScore = topicData.value("coherenceScore", 0.8);
            cluster.searchFrequency = topicData.value("searchFrequency", 0);
            
            if (topicData.contains("verseKeys")) {
                for (const auto& verseKey : topicData["verseKeys"]) {
                    cluster.verseKeys.insert(verseKey);
                }
            }
            
            topics[topicName] = cluster;
        }
    } catch (const json::exception& e) {
        // Handle JSON parsing error gracefully
    }
}