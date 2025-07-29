#ifndef USERSETTINGS_H
#define USERSETTINGS_H

#include <string>
#include <vector>
#include <unordered_map>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

struct DisplaySettings {
    float fontSize = 16.0f;
    std::string fontFamily = "default";
    std::string colorTheme = "dark";
    std::string highlightColor = "#FFD700"; // Gold
    std::string backgroundColor = "#1E1E1E";
    std::string textColor = "#FFFFFF";
    int windowWidth = 1400;
    int windowHeight = 900;
    int windowPosX = -1; // -1 means center
    int windowPosY = -1; // -1 means center
    bool rememberWindowState = true;
};

struct SearchSettings {
    std::string defaultTranslation = "KJV";
    int maxSearchResults = 50;
    bool fuzzySearchEnabled = false;
    bool autoSearch = true;
    std::string searchResultFormat = "reference_text"; // "reference_text", "text_only", "reference_only"
    bool showPerformanceStats = false;
};

struct ContentSettings {
    std::vector<std::string> favoriteVerses;
    std::vector<std::string> searchHistory;
    std::vector<std::string> recentTranslations;
    std::unordered_map<std::string, std::string> customBookAliases;
    std::string verseDisplayFormat = "standard"; // "standard", "compact", "detailed"
    int maxHistoryEntries = 100;
    bool saveSearchHistory = true;
};

struct UserSettings {
    DisplaySettings display;
    SearchSettings search;
    ContentSettings content;
    std::string version = "1.0";
    
    // Serialization methods
    json toJson() const;
    void fromJson(const json& j);
    
    // Validation methods
    bool validate() const;
    void applyDefaults();
    
    // Utility methods
    void addToSearchHistory(const std::string& query);
    void addToRecentTranslations(const std::string& translation);
    void addFavoriteVerse(const std::string& verse);
    void removeFavoriteVerse(const std::string& verse);
    bool isFavoriteVerse(const std::string& verse) const;
};

// JSON serialization support
void to_json(json& j, const DisplaySettings& settings);
void from_json(const json& j, DisplaySettings& settings);

void to_json(json& j, const SearchSettings& settings);
void from_json(const json& j, SearchSettings& settings);

void to_json(json& j, const ContentSettings& settings);
void from_json(const json& j, ContentSettings& settings);

void to_json(json& j, const UserSettings& settings);
void from_json(const json& j, UserSettings& settings);

#endif //USERSETTINGS_H