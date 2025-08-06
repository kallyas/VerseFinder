#ifndef SETTINGS_MODAL_H
#define SETTINGS_MODAL_H

#include <imgui.h>
#include <string>
#include <vector>
#include <memory>
#include "../../core/UserSettings.h"
#include "../../core/VerseFinder.h"
#include "../../core/HttpClient.h"
#include "../common/TranslationManager.h"

class SettingsModal {
public:
    SettingsModal(UserSettings& settings, VerseFinder* verse_finder);
    ~SettingsModal();

    void render(bool& show_window);
    
    // Settings management
    void saveSettings();
    void loadSettings();
    void exportSettings();
    void importSettings();
    void resetToDefaults();
    
    // Translation management
    void downloadTranslation(const std::string& url, const std::string& name);
    void updateAvailableTranslationStatus();
    void fetchAvailableTranslations();
    void refreshTranslationsList();
    
private:
    UserSettings& userSettings;
    VerseFinder* verse_finder;
    
    // HTTP client for API requests
    std::unique_ptr<HttpClient> http_client;
    
    // Available translations
    std::vector<DownloadableTranslation> available_translations;
    bool translations_loading;
    bool translations_fetched;
    std::string translations_error;
    
    // UI state
    char custom_url_input[512];
    char custom_name_input[256];
    
    // Tab rendering methods
    void renderTranslationsTab();
    void renderAppearanceTab();
    void renderContentTab();
    void renderAccessibilityTab();
    
    // Settings sections
    void renderFontSettings();
    void renderColorThemeSettings();
    void renderWindowSettings();
    void renderSearchSettings();
    void renderDisplaySettings();
    void renderAdvancedOptions();
    
    // Utility methods
    void applyThemeChange();
    std::string getSettingsFilePath() const;
    void initializeAvailableTranslations();
    
    // HTTP utilities for translation downloads
    void downloadFromUrl(const std::string& url, const std::string& filename);
};

#endif // SETTINGS_MODAL_H