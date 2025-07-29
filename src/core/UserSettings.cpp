#include "UserSettings.h"
#include <algorithm>
#include <iostream>

// DisplaySettings JSON serialization
void to_json(json& j, const DisplaySettings& settings) {
    j = json{
        {"fontSize", settings.fontSize},
        {"fontFamily", settings.fontFamily},
        {"colorTheme", settings.colorTheme},
        {"highlightColor", settings.highlightColor},
        {"backgroundColor", settings.backgroundColor},
        {"textColor", settings.textColor},
        {"windowWidth", settings.windowWidth},
        {"windowHeight", settings.windowHeight},
        {"windowPosX", settings.windowPosX},
        {"windowPosY", settings.windowPosY},
        {"rememberWindowState", settings.rememberWindowState}
    };
}

void from_json(const json& j, DisplaySettings& settings) {
    j.at("fontSize").get_to(settings.fontSize);
    j.at("fontFamily").get_to(settings.fontFamily);
    j.at("colorTheme").get_to(settings.colorTheme);
    j.at("highlightColor").get_to(settings.highlightColor);
    j.at("backgroundColor").get_to(settings.backgroundColor);
    j.at("textColor").get_to(settings.textColor);
    j.at("windowWidth").get_to(settings.windowWidth);
    j.at("windowHeight").get_to(settings.windowHeight);
    j.at("windowPosX").get_to(settings.windowPosX);
    j.at("windowPosY").get_to(settings.windowPosY);
    j.at("rememberWindowState").get_to(settings.rememberWindowState);
}

// SearchSettings JSON serialization
void to_json(json& j, const SearchSettings& settings) {
    j = json{
        {"defaultTranslation", settings.defaultTranslation},
        {"maxSearchResults", settings.maxSearchResults},
        {"fuzzySearchEnabled", settings.fuzzySearchEnabled},
        {"autoSearch", settings.autoSearch},
        {"searchResultFormat", settings.searchResultFormat},
        {"showPerformanceStats", settings.showPerformanceStats}
    };
}

void from_json(const json& j, SearchSettings& settings) {
    j.at("defaultTranslation").get_to(settings.defaultTranslation);
    j.at("maxSearchResults").get_to(settings.maxSearchResults);
    j.at("fuzzySearchEnabled").get_to(settings.fuzzySearchEnabled);
    j.at("autoSearch").get_to(settings.autoSearch);
    j.at("searchResultFormat").get_to(settings.searchResultFormat);
    j.at("showPerformanceStats").get_to(settings.showPerformanceStats);
}

// ContentSettings JSON serialization
void to_json(json& j, const ContentSettings& settings) {
    j = json{
        {"favoriteVerses", settings.favoriteVerses},
        {"searchHistory", settings.searchHistory},
        {"recentTranslations", settings.recentTranslations},
        {"customBookAliases", settings.customBookAliases},
        {"verseDisplayFormat", settings.verseDisplayFormat},
        {"maxHistoryEntries", settings.maxHistoryEntries},
        {"saveSearchHistory", settings.saveSearchHistory}
    };
}

void from_json(const json& j, ContentSettings& settings) {
    j.at("favoriteVerses").get_to(settings.favoriteVerses);
    j.at("searchHistory").get_to(settings.searchHistory);
    j.at("recentTranslations").get_to(settings.recentTranslations);
    j.at("customBookAliases").get_to(settings.customBookAliases);
    j.at("verseDisplayFormat").get_to(settings.verseDisplayFormat);
    j.at("maxHistoryEntries").get_to(settings.maxHistoryEntries);
    j.at("saveSearchHistory").get_to(settings.saveSearchHistory);
}

// UserSettings JSON serialization
void to_json(json& j, const UserSettings& settings) {
    j = json{
        {"version", settings.version},
        {"display", settings.display},
        {"search", settings.search},
        {"content", settings.content}
    };
}

void from_json(const json& j, UserSettings& settings) {
    j.at("version").get_to(settings.version);
    j.at("display").get_to(settings.display);
    j.at("search").get_to(settings.search);
    j.at("content").get_to(settings.content);
}

// UserSettings implementation
json UserSettings::toJson() const {
    json j;
    to_json(j, *this);
    return j;
}

void UserSettings::fromJson(const json& j) {
    from_json(j, *this);
}

bool UserSettings::validate() const {
    // Validate display settings
    if (display.fontSize < 8.0f || display.fontSize > 72.0f) {
        return false;
    }
    if (display.windowWidth < 400 || display.windowHeight < 300) {
        return false;
    }
    
    // Validate search settings
    if (search.maxSearchResults < 1 || search.maxSearchResults > 1000) {
        return false;
    }
    
    // Validate content settings
    if (content.maxHistoryEntries < 0 || content.maxHistoryEntries > 10000) {
        return false;
    }
    
    return true;
}

void UserSettings::applyDefaults() {
    display = DisplaySettings{};
    search = SearchSettings{};
    content = ContentSettings{};
    version = "1.0";
}

void UserSettings::addToSearchHistory(const std::string& query) {
    if (!content.saveSearchHistory || query.empty()) {
        return;
    }
    
    // Remove if already exists to move to front
    auto it = std::find(content.searchHistory.begin(), content.searchHistory.end(), query);
    if (it != content.searchHistory.end()) {
        content.searchHistory.erase(it);
    }
    
    // Add to front
    content.searchHistory.insert(content.searchHistory.begin(), query);
    
    // Limit size
    if (content.searchHistory.size() > static_cast<size_t>(content.maxHistoryEntries)) {
        content.searchHistory.resize(content.maxHistoryEntries);
    }
}

void UserSettings::addToRecentTranslations(const std::string& translation) {
    if (translation.empty()) {
        return;
    }
    
    // Remove if already exists
    auto it = std::find(content.recentTranslations.begin(), content.recentTranslations.end(), translation);
    if (it != content.recentTranslations.end()) {
        content.recentTranslations.erase(it);
    }
    
    // Add to front
    content.recentTranslations.insert(content.recentTranslations.begin(), translation);
    
    // Limit to 10 recent translations
    if (content.recentTranslations.size() > 10) {
        content.recentTranslations.resize(10);
    }
}

void UserSettings::addFavoriteVerse(const std::string& verse) {
    if (verse.empty() || isFavoriteVerse(verse)) {
        return;
    }
    content.favoriteVerses.push_back(verse);
}

void UserSettings::removeFavoriteVerse(const std::string& verse) {
    auto it = std::find(content.favoriteVerses.begin(), content.favoriteVerses.end(), verse);
    if (it != content.favoriteVerses.end()) {
        content.favoriteVerses.erase(it);
    }
}

bool UserSettings::isFavoriteVerse(const std::string& verse) const {
    return std::find(content.favoriteVerses.begin(), content.favoriteVerses.end(), verse) != content.favoriteVerses.end();
}