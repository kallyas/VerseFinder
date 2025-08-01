#include "SettingsModal.h"
#include <imgui.h>
#include <iostream>

SettingsModal::SettingsModal(UserSettings& settings, VerseFinder* verse_finder)
    : userSettings(settings), verse_finder(verse_finder) {
    memset(custom_url_input, 0, sizeof(custom_url_input));
    memset(custom_name_input, 0, sizeof(custom_name_input));
    initializeAvailableTranslations();
}

SettingsModal::~SettingsModal() {
}

void SettingsModal::render(bool& show_window) {
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Settings", &show_window)) {
        if (ImGui::BeginTabBar("SettingsTabs")) {
            if (ImGui::BeginTabItem("Translations")) {
                renderTranslationsTab();
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Appearance")) {
                renderAppearanceTab();
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Content")) {
                renderContentTab();
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
        
        ImGui::Separator();
        ImGui::Spacing();
        
        // Settings management buttons
        if (ImGui::Button("Save Settings", ImVec2(120, 0))) {
            saveSettings();
        }
        ImGui::SameLine();
        if (ImGui::Button("Export", ImVec2(120, 0))) {
            exportSettings();
        }
        ImGui::SameLine();
        if (ImGui::Button("Import", ImVec2(120, 0))) {
            importSettings();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset to Defaults", ImVec2(150, 0))) {
            resetToDefaults();
        }
    }
    ImGui::End();
}

void SettingsModal::renderTranslationsTab() {
    ImGui::Text("Manage Bible translations for VerseFinder");
    ImGui::Separator();
    
    // Download all button
    if (ImGui::Button("Download All Free Translations", ImVec2(-1, 30))) {
        for (auto& trans : available_translations) {
            if (!trans.is_downloaded && !trans.is_downloading) {
                downloadTranslation(trans.url, trans.name);
            }
        }
    }
    
    ImGui::Spacing();
    
    // Translation list
    if (ImGui::BeginTable("TranslationsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Translation");
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("Description");
        ImGui::TableSetupColumn("Actions");
        ImGui::TableHeadersRow();
        
        for (auto& trans : available_translations) {
            ImGui::TableNextRow();
            
            // Name
            ImGui::TableNextColumn();
            ImGui::Text("%s (%s)", trans.name.c_str(), trans.abbreviation.c_str());
            
            // Status
            ImGui::TableNextColumn();
            if (trans.is_downloading) {
                ImGui::ProgressBar(trans.download_progress);
            } else if (trans.is_downloaded) {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Downloaded");
            } else {
                ImGui::Text("Available");
            }
            
            // Description
            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", trans.description.c_str());
            
            // Actions
            ImGui::TableNextColumn();
            if (!trans.is_downloaded && !trans.is_downloading) {
                if (ImGui::Button(("Download##" + trans.abbreviation).c_str())) {
                    downloadTranslation(trans.url, trans.name);
                }
            }
        }
        ImGui::EndTable();
    }
}

void SettingsModal::renderAppearanceTab() {
    ImGui::Text("Font Settings");
    ImGui::Separator();
    
    // Font size slider
    ImGui::SliderFloat("Font Size", &userSettings.display.fontSize, 10.0f, 24.0f, "%.1f");
    
    ImGui::Spacing();
    ImGui::Text("Color Theme");
    ImGui::Separator();
    
    // Theme selection
    const char* themes[] = {"dark", "light", "blue", "green"};
    int current_theme = 0;
    for (int i = 0; i < 4; i++) {
        if (userSettings.display.colorTheme == themes[i]) {
            current_theme = i;
            break;
        }
    }
    
    if (ImGui::Combo("Theme", &current_theme, themes, 4)) {
        userSettings.display.colorTheme = themes[current_theme];
        applyThemeChange();
    }
    
    ImGui::Spacing();
    ImGui::Text("Window Settings");
    ImGui::Separator();
    
    ImGui::Checkbox("Remember window state", &userSettings.display.rememberWindowState);
    if (userSettings.display.rememberWindowState) {
        ImGui::SliderInt("Window Width", &userSettings.display.windowWidth, 800, 1920);
        ImGui::SliderInt("Window Height", &userSettings.display.windowHeight, 600, 1080);
    }
}

void SettingsModal::renderContentTab() {
    ImGui::Text("Search History");
    ImGui::Separator();
    
    ImGui::Checkbox("Save search history", &userSettings.content.saveSearchHistory);
    if (userSettings.content.saveSearchHistory) {
        ImGui::SliderInt("Max history entries", &userSettings.content.maxHistoryEntries, 10, 100);
        
        if (ImGui::Button("Clear History")) {
            userSettings.content.searchHistory.clear();
        }
    }
    
    ImGui::Spacing();
    ImGui::Text("Verse Display");
    ImGui::Separator();
    
    // Search result format
    const char* formats[] = {"reference_text", "text_only", "reference_only"};
    int current_format = 0;
    for (int i = 0; i < 3; i++) {
        if (userSettings.search.searchResultFormat == formats[i]) {
            current_format = i;
            break;
        }
    }
    if (ImGui::Combo("Result Format", &current_format, formats, 3)) {
        userSettings.search.searchResultFormat = formats[current_format];
    }
    
    ImGui::SliderInt("Max search results", &userSettings.search.maxSearchResults, 10, 200);
}

void SettingsModal::saveSettings() {
    // Implementation would save to file
    std::cout << "Settings saved (implementation needed)" << std::endl;
}

void SettingsModal::loadSettings() {
    // Implementation would load from file
    std::cout << "Settings loaded (implementation needed)" << std::endl;
}

void SettingsModal::exportSettings() {
    std::cout << "Export settings (implementation needed)" << std::endl;
}

void SettingsModal::importSettings() {
    std::cout << "Import settings (implementation needed)" << std::endl;
}

void SettingsModal::resetToDefaults() {
    userSettings.applyDefaults();
    applyThemeChange();
}

void SettingsModal::downloadTranslation(const std::string& url, const std::string& name) {
    std::cout << "Downloading translation: " << name << " from " << url << std::endl;
    // Implementation would download translation
}

void SettingsModal::updateAvailableTranslationStatus() {
    // Implementation would check for existing translations
}

void SettingsModal::applyThemeChange() {
    // Implementation would apply theme change
    std::cout << "Theme changed to: " << userSettings.display.colorTheme << std::endl;
}

std::string SettingsModal::getSettingsFilePath() const {
    return "settings.json";
}

void SettingsModal::initializeAvailableTranslations() {
    available_translations = {
        {"King James Version", "KJV", "https://api.getbible.net/v2/kjv.json", 
         "The classic English translation from 1611", true, false, 1.0f},
        {"New International Version", "NIV", "https://api.getbible.net/v2/niv.json", 
         "Modern English translation, widely used", false, false, 0.0f},
        {"English Standard Version", "ESV", "https://api.getbible.net/v2/esv.json", 
         "Literal yet readable modern translation", false, false, 0.0f},
        {"New Living Translation", "NLT", "https://api.getbible.net/v2/nlt.json", 
         "Thought-for-thought contemporary translation", false, false, 0.0f},
        {"American Standard Version", "ASV", "https://api.getbible.net/v2/asv.json", 
         "Classic American revision of the KJV", false, false, 0.0f},
        {"World English Bible", "WEB", "https://api.getbible.net/v2/web.json", 
         "Modern public domain translation", false, false, 0.0f}
    };
}

void SettingsModal::downloadFromUrl(const std::string& url, const std::string& filename) {
    // Implementation would perform HTTP download
    std::cout << "Would download from " << url << " to " << filename << std::endl;
}

size_t SettingsModal::writeCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t total_size = size * nmemb;
    response->append((char*)contents, total_size);
    return total_size;
}