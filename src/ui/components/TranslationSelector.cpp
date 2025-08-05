#include "TranslationSelector.h"
#include <imgui.h>

TranslationSelector::TranslationSelector() 
    : current_translation("KJV") {
    initializeDefaultTranslations();
}

TranslationSelector::~TranslationSelector() {
}

void TranslationSelector::render() {
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Translation");
    ImGui::SameLine();
    
    // Translation dropdown
    if (ImGui::BeginCombo("##translation", current_translation.c_str())) {
        for (const auto& translation : available_translations) {
            if (translation.is_loaded) {
                bool is_selected = (current_translation == translation.abbreviation);
                std::string display_text = translation.abbreviation + " - " + translation.name;
                if (!translation.description.empty()) {
                    display_text += " (" + translation.description + ")";
                }
                
                if (ImGui::Selectable(display_text.c_str(), is_selected)) {
                    current_translation = translation.abbreviation;
                    if (on_translation_changed) {
                        on_translation_changed(current_translation);
                    }
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
        }
        ImGui::EndCombo();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Manage Translations")) {
        if (on_manage_translations) {
            on_manage_translations();
        }
    }
}

void TranslationSelector::setAvailableTranslations(const std::vector<TranslationInfo>& translations) {
    available_translations = translations;
}

void TranslationSelector::setCurrentTranslation(const std::string& translation_name) {
    current_translation = translation_name;
}

void TranslationSelector::initializeDefaultTranslations() {
    available_translations = {
        TranslationInfo("King James Version", "KJV", "The classic English translation from 1611", 1611, "English"),
        TranslationInfo("American Standard Version", "ASV", "Classic American revision of the KJV", 1901, "English"),
        TranslationInfo("World English Bible", "WEB", "Modern public domain translation", 2000, "English"),
        TranslationInfo("American King James Version", "AKJV", "Updated spelling and vocabulary of the KJV", 1999, "English"),
        TranslationInfo("Basic English Bible", "BBE", "Simple English translation using basic vocabulary", 1965, "English")
    };
}