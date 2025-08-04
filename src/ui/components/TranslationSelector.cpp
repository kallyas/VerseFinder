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
            if (translation.is_downloaded) {
                bool is_selected = (current_translation == translation.abbreviation);
                if (ImGui::Selectable((translation.abbreviation + " - " + translation.name).c_str(), is_selected)) {
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

void TranslationSelector::setAvailableTranslations(const std::vector<DownloadableTranslation>& translations) {
    available_translations = translations;
}

void TranslationSelector::setCurrentTranslation(const std::string& translation_name) {
    current_translation = translation_name;
}

void TranslationSelector::initializeDefaultTranslations() {
    available_translations = {
        {"King James Version", "KJV", "https://api.getbible.net/v2/kjv.json", 
         "The classic English translation from 1611", true, false, 1.0f},
        {"American Standard Version", "ASV", "https://api.getbible.net/v2/asv.json", 
         "Classic American revision of the KJV", false, false, 0.0f},
        {"World English Bible", "WEB", "https://api.getbible.net/v2/web.json", 
         "Modern public domain translation", false, false, 0.0f},
        {"American King James Version", "AKJV", "https://api.getbible.net/v2/akjv.json",
         "Updated spelling and vocabulary of the KJV", false, false, 0.0f},
        {"Basic English Bible", "BBE", "https://api.getbible.net/v2/basicenglish.json",
         "Simple English translation using basic vocabulary", false, false, 0.0f}
    };
}