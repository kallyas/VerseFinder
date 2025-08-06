#ifndef PLUGIN_API_H
#define PLUGIN_API_H

#include "PluginInterfaces.h"
#include "../../core/VerseFinder.h"
#include <functional>
#include <memory>

namespace PluginSystem {

// Event system for plugin communication
class PluginEvent {
public:
    std::string type;
    std::unordered_map<std::string, std::string> data;
    std::string source;
    std::chrono::steady_clock::time_point timestamp;
    
    PluginEvent(const std::string& eventType, const std::string& sourcePlugin = "")
        : type(eventType), source(sourcePlugin), timestamp(std::chrono::steady_clock::now()) {}
    
    void setData(const std::string& key, const std::string& value) {
        data[key] = value;
    }
    
    std::string getData(const std::string& key, const std::string& defaultValue = "") const {
        auto it = data.find(key);
        return it != data.end() ? it->second : defaultValue;
    }
};

// Event listener interface
using EventCallback = std::function<void(const PluginEvent&)>;

// Plugin API - provides access to VerseFinder functionality
class PluginAPI {
private:
    VerseFinder* bible_instance;
    std::unordered_map<std::string, std::vector<EventCallback>> event_listeners;
    
public:
    explicit PluginAPI(VerseFinder* bible) : bible_instance(bible) {}
    
    // Bible search methods
    std::string searchByReference(const std::string& reference, const std::string& translation) const {
        return bible_instance ? bible_instance->searchByReference(reference, translation) : "";
    }
    
    std::vector<std::string> searchByKeywords(const std::string& query, const std::string& translation) const {
        return bible_instance ? bible_instance->searchByKeywords(query, translation) : std::vector<std::string>();
    }
    
    std::vector<std::string> searchByChapter(const std::string& reference, const std::string& translation) const {
        return bible_instance ? bible_instance->searchByChapter(reference, translation) : std::vector<std::string>();
    }
    
    std::vector<std::string> searchSemantic(const std::string& query, const std::string& translation) const {
        return bible_instance ? bible_instance->searchSemantic(query, translation) : std::vector<std::string>();
    }
    
    // Translation management
    const std::vector<TranslationInfo>& getTranslations() const {
        static std::vector<TranslationInfo> empty;
        return bible_instance ? bible_instance->getTranslations() : empty;
    }
    
    bool loadTranslationFromFile(const std::string& filename) {
        return bible_instance ? bible_instance->loadTranslationFromFile(filename) : false;
    }
    
    // Cross-references and context
    std::vector<std::string> findCrossReferences(const std::string& verseKey) const {
        return bible_instance ? bible_instance->findCrossReferences(verseKey) : std::vector<std::string>();
    }
    
    std::vector<std::string> expandVerseContext(const std::string& verseKey, int contextSize = 2) const {
        return bible_instance ? bible_instance->expandVerseContext(verseKey, contextSize) : std::vector<std::string>();
    }
    
    // Auto-complete and suggestions
    std::vector<std::string> getAutoCompletions(const std::string& input, int maxResults = 10) const {
        return bible_instance ? bible_instance->getAutoCompletions(input, maxResults) : std::vector<std::string>();
    }
    
    std::vector<std::string> getSmartSuggestions(const std::string& input, int maxResults = 10) const {
        return bible_instance ? bible_instance->getSmartSuggestions(input, maxResults) : std::vector<std::string>();
    }
    
    // Favorites and collections
    void addToFavorites(const std::string& verseKey) {
        if (bible_instance) bible_instance->addToFavorites(verseKey);
    }
    
    void removeFromFavorites(const std::string& verseKey) {
        if (bible_instance) bible_instance->removeFromFavorites(verseKey);
    }
    
    std::vector<std::string> getFavoriteVerses() const {
        return bible_instance ? bible_instance->getFavoriteVerses() : std::vector<std::string>();
    }
    
    bool isFavorite(const std::string& verseKey) const {
        return bible_instance ? bible_instance->isFavorite(verseKey) : false;
    }
    
    // Collections
    void createCollection(const std::string& name, const std::vector<std::string>& verses) {
        if (bible_instance) bible_instance->createCollection(name, verses);
    }
    
    std::vector<std::string> getCollection(const std::string& name) const {
        return bible_instance ? bible_instance->getCollection(name) : std::vector<std::string>();
    }
    
    std::vector<std::string> getAllCollections() const {
        return bible_instance ? bible_instance->getAllCollections() : std::vector<std::string>();
    }
    
    // Topic management
    std::vector<std::string> getVersesByTopic(const std::string& topic, int maxResults = 50) const {
        return bible_instance ? bible_instance->getVersesByTopic(topic, maxResults) : std::vector<std::string>();
    }
    
    std::vector<std::string> getRelatedTopics(const std::string& topic, int maxResults = 10) const {
        return bible_instance ? bible_instance->getRelatedTopics(topic, maxResults) : std::vector<std::string>();
    }
    
    // Analytics and discovery
    std::string getVerseOfTheDay() const {
        return bible_instance ? bible_instance->getVerseOfTheDay() : "";
    }
    
    std::string getRandomVerse() const {
        return bible_instance ? bible_instance->getRandomVerse() : "";
    }
    
    std::vector<std::string> getPopularVerses(int count = 10) const {
        return bible_instance ? bible_instance->getPopularVerses(count) : std::vector<std::string>();
    }
    
    // Event system
    void addEventListener(const std::string& eventType, EventCallback callback) {
        event_listeners[eventType].push_back(callback);
    }
    
    void removeEventListener(const std::string& eventType) {
        event_listeners.erase(eventType);
    }
    
    void triggerEvent(const PluginEvent& event) {
        auto it = event_listeners.find(event.type);
        if (it != event_listeners.end()) {
            for (const auto& callback : it->second) {
                try {
                    callback(event);
                } catch (...) {
                    // Log error but don't crash
                }
            }
        }
    }
    
    // Utility methods
    bool parseReference(const std::string& reference, std::string& book, int& chapter, int& verse) const {
        return bible_instance ? bible_instance->parseReference(reference, book, chapter, verse) : false;
    }
    
    std::string normalizeBookName(const std::string& book) const {
        return bible_instance ? bible_instance->normalizeBookName(book) : "";
    }
    
    bool verseExists(const std::string& book, int chapter, int verse, const std::string& translation) const {
        return bible_instance ? bible_instance->verseExists(book, chapter, verse, translation) : false;
    }
    
    // Performance monitoring
    void recordSearch(const std::string& query, const std::string& queryType, int resultCount, double executionTime) {
        if (bible_instance) bible_instance->recordSearch(query, queryType, resultCount, executionTime);
    }
    
    void recordVerseSelection(const std::string& query, const std::string& verseKey) {
        if (bible_instance) bible_instance->recordVerseSelection(query, verseKey);
    }
};

// Predefined event types
namespace Events {
    constexpr const char* VERSE_SELECTED = "verse_selected";
    constexpr const char* SEARCH_PERFORMED = "search_performed";
    constexpr const char* TRANSLATION_CHANGED = "translation_changed";
    constexpr const char* FAVORITES_UPDATED = "favorites_updated";
    constexpr const char* COLLECTION_CREATED = "collection_created";
    constexpr const char* PLUGIN_LOADED = "plugin_loaded";
    constexpr const char* PLUGIN_UNLOADED = "plugin_unloaded";
    constexpr const char* SETTINGS_CHANGED = "settings_changed";
    constexpr const char* PRESENTATION_MODE_CHANGED = "presentation_mode_changed";
    constexpr const char* UI_THEME_CHANGED = "ui_theme_changed";
}

} // namespace PluginSystem

#endif // PLUGIN_API_H