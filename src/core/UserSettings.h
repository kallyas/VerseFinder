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

struct PresentationSettings {
    bool enabled = false;
    float fontSize = 48.0f;
    std::string fontFamily = "Arial";
    std::string backgroundColor = "#000000"; // Black
    std::string textColor = "#FFFFFF"; // White
    std::string referenceColor = "#CCCCCC"; // Light gray
    bool showReference = true;
    bool showBackground = false;
    std::string backgroundImagePath = "";
    int windowWidth = 1920;
    int windowHeight = 1080;
    int windowPosX = -1; // -1 means auto-detect second monitor
    int windowPosY = -1;
    int monitorIndex = 1; // 0 = primary, 1 = secondary
    bool fullscreen = true;
    bool obsOptimized = true; // Optimizations for OBS capture
    std::string windowTitle = "VerseFinder - Presentation";
    bool autoHideCursor = true;
    float fadeTransitionTime = 0.3f; // seconds
    std::string textAlignment = "center"; // "left", "center", "right"
    float textPadding = 40.0f; // pixels from edge
};

struct UserSettings {
    DisplaySettings display;
    SearchSettings search;
    ContentSettings content;
    PresentationSettings presentation;
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

void to_json(json& j, const PresentationSettings& settings);
void from_json(const json& j, PresentationSettings& settings);

void to_json(json& j, const UserSettings& settings);
void from_json(const json& j, UserSettings& settings);

#endif //USERSETTINGS_H