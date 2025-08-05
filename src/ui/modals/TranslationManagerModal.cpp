#include "TranslationManagerModal.h"
#include <imgui.h>
#include <iostream>
#include <filesystem>

TranslationManagerModal::TranslationManagerModal() 
    : verse_finder(nullptr) {
}

TranslationManagerModal::~TranslationManagerModal() {
}

void TranslationManagerModal::render() {
    if (!show_modal) return;
    
    if (!verse_finder) {
        show_modal = false;
        return;
    }
    
    ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Translation Manager", &show_modal)) {
        ImGui::Text("Manage Bible Translations");
        ImGui::Separator();
        
        if (ImGui::BeginTabBar("TranslationManagerTabs")) {
            if (ImGui::BeginTabItem("Loaded Translations")) {
                renderTranslationList();
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Add Translation")) {
                renderAddTranslation();
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
        
        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(100, 0))) {
            show_modal = false;
        }
    }
    ImGui::End();
}

void TranslationManagerModal::renderTranslationList() {
    const auto& translations = verse_finder->getTranslations();
    
    if (translations.empty()) {
        ImGui::Text("No translations loaded.");
        ImGui::Text("Use the 'Add Translation' tab to load Bible translations.");
        return;
    }
    
    ImGui::Text("Currently loaded translations:");
    ImGui::Spacing();
    
    if (ImGui::BeginTable("TranslationsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Abbreviation");
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Description");
        ImGui::TableSetupColumn("Year");
        ImGui::TableSetupColumn("Status");
        ImGui::TableHeadersRow();
        
        for (const auto& trans : translations) {
            ImGui::TableNextRow();
            
            ImGui::TableNextColumn();
            ImGui::Text("%s", trans.abbreviation.c_str());
            
            ImGui::TableNextColumn();
            ImGui::Text("%s", trans.name.c_str());
            
            ImGui::TableNextColumn();
            ImGui::Text("%s", trans.description.c_str());
            
            ImGui::TableNextColumn();
            if (trans.year > 0) {
                ImGui::Text("%d", trans.year);
            } else {
                ImGui::Text("-");
            }
            
            ImGui::TableNextColumn();
            if (trans.is_loaded) {
                ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "Loaded");
            } else {
                ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.0f, 1.0f), "Not Loaded");
            }
        }
        
        ImGui::EndTable();
    }
    
    ImGui::Spacing();
    if (ImGui::Button("Refresh List", ImVec2(120, 0))) {
        refreshTranslationList();
    }
}

void TranslationManagerModal::renderAddTranslation() {
    ImGui::Text("Add a new Bible translation");
    ImGui::Spacing();
    
    ImGui::Text("Load from JSON file:");
    renderFilePathInput();
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::Text("Instructions:");
    ImGui::BulletText("Select a JSON file containing Bible translation data");
    ImGui::BulletText("The file should follow the standard format with translation metadata");
    ImGui::BulletText("Supported formats: JSON with 'translation', 'abbreviation', 'books' fields");
    
    ImGui::Spacing();
    ImGui::Text("Expected JSON structure:");
    ImGui::Text("{\n"
               "  \"translation\": \"Translation Name\",\n"
               "  \"abbreviation\": \"ABBR\",\n"
               "  \"description\": \"Description\",\n"
               "  \"year\": 2000,\n"
               "  \"language\": \"English\",\n"
               "  \"books\": [...]\n"
               "}");
}

void TranslationManagerModal::renderFilePathInput() {
    ImGui::Text("File path:");
    ImGui::PushItemWidth(400);
    ImGui::InputText("##filepath", path_buffer, sizeof(path_buffer));
    ImGui::PopItemWidth();
    
    ImGui::SameLine();
    if (ImGui::Button("Browse...")) {
        // In a real implementation, this would open a file dialog
        // For now, we'll provide a simple input
        show_file_browser = !show_file_browser;
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        std::string file_path(path_buffer);
        if (!file_path.empty()) {
            if (loadTranslationFromFile(file_path)) {
                ImGui::OpenPopup("Success");
                // Clear the input
                path_buffer[0] = '\0';
            } else {
                ImGui::OpenPopup("Error");
            }
        }
    }
    
    // Simple file browser substitute
    if (show_file_browser) {
        ImGui::Spacing();
        ImGui::Text("Quick paths:");
        if (ImGui::Button("./bible.json")) {
            strcpy(path_buffer, "./bible.json");
            show_file_browser = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("./translations/")) {
            strcpy(path_buffer, "./translations/");
            show_file_browser = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            show_file_browser = false;
        }
    }
    
    // Success/Error popups
    if (ImGui::BeginPopupModal("Success", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Translation loaded successfully!");
        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
            if (on_translation_changed) {
                on_translation_changed();
            }
        }
        ImGui::EndPopup();
    }
    
    if (ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Failed to load translation file.");
        ImGui::Text("Please check the file path and format.");
        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

bool TranslationManagerModal::loadTranslationFromFile(const std::string& file_path) {
    if (!verse_finder) return false;
    
    try {
        // Check if file exists
        if (!std::filesystem::exists(file_path)) {
            std::cerr << "File does not exist: " << file_path << std::endl;
            return false;
        }
        
        // Load translation using VerseFinder's public method
        return verse_finder->loadTranslationFromFile(file_path);
    } catch (const std::exception& e) {
        std::cerr << "Error loading translation: " << e.what() << std::endl;
        return false;
    }
}

void TranslationManagerModal::refreshTranslationList() {
    // This would trigger a refresh of the translation list
    // In the current implementation, the list is automatically updated
    // when new translations are loaded
    if (on_translation_changed) {
        on_translation_changed();
    }
}