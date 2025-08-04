#include "TranslationComparison.h"
#include <imgui.h>
#include <algorithm>
#include <sstream>
#include <cctype>

TranslationComparison::TranslationComparison() 
    : verse_finder(nullptr), current_reference("John 3:16") {
}

TranslationComparison::~TranslationComparison() {
}

void TranslationComparison::render() {
    if (!verse_finder) return;
    
    if (ImGui::BeginChild("TranslationComparison", ImVec2(0, comparison_height), true)) {
        ImGui::Text("Translation Comparison");
        ImGui::Separator();
        
        // Controls
        ImGui::Checkbox("Show word differences", &show_word_differences);
        ImGui::SameLine();
        ImGui::Checkbox("Show metadata", &show_metadata);
        
        ImGui::Spacing();
        
        // Reference input with example suggestions
        char ref_buffer[256];
        strncpy(ref_buffer, current_reference.c_str(), sizeof(ref_buffer) - 1);
        ref_buffer[sizeof(ref_buffer) - 1] = '\0';
        
        ImGui::PushItemWidth(300);
        if (ImGui::InputText("Reference", ref_buffer, sizeof(ref_buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
            current_reference = std::string(ref_buffer);
            updateComparison();
            if (on_reference_changed) {
                on_reference_changed(current_reference);
            }
        }
        ImGui::PopItemWidth();
        
        // Quick reference buttons
        ImGui::SameLine();
        if (ImGui::Button("John 3:16")) {
            current_reference = "John 3:16";
            updateComparison();
        }
        ImGui::SameLine();
        if (ImGui::Button("Psalm 23:1")) {
            current_reference = "Psalm 23:1";
            updateComparison();
        }
        ImGui::SameLine();
        if (ImGui::Button("Romans 8:28")) {
            current_reference = "Romans 8:28";
            updateComparison();
        }
        
        ImGui::Spacing();
        
        // Translation selection
        const auto& available_translations = verse_finder->getTranslations();
        if (available_translations.empty()) {
            ImGui::Text("No translations loaded.");
            ImGui::EndChild();
            return;
        }
        
        // Ensure we have some translations selected
        if (selected_translations.empty()) {
            for (size_t i = 0; i < std::min(size_t(2), available_translations.size()); ++i) {
                if (available_translations[i].is_loaded) {
                    selected_translations.push_back(available_translations[i].name);
                }
            }
            updateComparison();
        }
        
        // Translation selection checkboxes
        ImGui::Text("Select translations to compare:");
        bool translation_changed = false;
        for (const auto& trans_info : available_translations) {
            if (!trans_info.is_loaded) continue;
            
            bool is_selected = std::find(selected_translations.begin(), 
                                       selected_translations.end(), 
                                       trans_info.name) != selected_translations.end();
            
            if (ImGui::Checkbox((trans_info.abbreviation + " - " + trans_info.name).c_str(), &is_selected)) {
                if (is_selected) {
                    if (selected_translations.size() < 4) { // Limit to 4 translations
                        selected_translations.push_back(trans_info.name);
                        translation_changed = true;
                    }
                } else {
                    selected_translations.erase(
                        std::remove(selected_translations.begin(), selected_translations.end(), trans_info.name),
                        selected_translations.end()
                    );
                    translation_changed = true;
                }
            }
        }
        
        if (translation_changed) {
            updateComparison();
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Render comparison results
        if (!current_comparison.translation_texts.empty()) {
            // Render translation panels side by side
            for (size_t i = 0; i < current_comparison.translation_texts.size(); ++i) {
                const auto& trans_text = current_comparison.translation_texts[i];
                const std::string& translation = trans_text.first;
                const std::string& text = trans_text.second;
                
                std::vector<bool> word_diffs;
                if (show_word_differences && i < current_comparison.word_differences.size()) {
                    word_diffs = current_comparison.word_differences[i];
                }
                
                if (i > 0) ImGui::SameLine();
                
                ImGui::BeginGroup();
                renderTranslationPanel(translation, text, word_diffs, i);
                ImGui::EndGroup();
                
                if (i < current_comparison.translation_texts.size() - 1) {
                    // Add some spacing between panels
                    ImGui::SameLine();
                    ImGui::Text("|");
                }
            }
        } else {
            ImGui::Text("No verse found for reference: %s", current_reference.c_str());
        }
    }
    ImGui::EndChild();
}

void TranslationComparison::setSelectedTranslations(const std::vector<std::string>& translations) {
    selected_translations = translations;
    updateComparison();
}

void TranslationComparison::setCurrentReference(const std::string& reference) {
    current_reference = reference;
    updateComparison();
}

void TranslationComparison::updateComparison() {
    if (!verse_finder || selected_translations.empty()) {
        current_comparison = ComparisonResult();
        return;
    }
    
    current_comparison = compareVerseTexts(current_reference);
}

ComparisonResult TranslationComparison::compareVerseTexts(const std::string& reference) {
    ComparisonResult result;
    result.reference = reference;
    
    std::vector<std::string> texts;
    
    // Collect verse texts from all selected translations
    for (const std::string& translation : selected_translations) {
        std::string verse_text = verse_finder->searchByReference(reference, translation);
        if (verse_text.find("Verse not found") == std::string::npos && 
            verse_text.find("Translation not found") == std::string::npos) {
            // Extract just the text part (remove reference prefix)
            size_t colon_pos = verse_text.find(": ");
            if (colon_pos != std::string::npos) {
                verse_text = verse_text.substr(colon_pos + 2);
            }
            result.translation_texts.push_back({translation, verse_text});
            texts.push_back(verse_text);
        }
    }
    
    // Analyze word differences
    if (show_word_differences && texts.size() > 1) {
        result.word_differences = analyzeWordDifferences(texts);
    }
    
    return result;
}

std::vector<std::vector<bool>> TranslationComparison::analyzeWordDifferences(const std::vector<std::string>& texts) {
    std::vector<std::vector<bool>> differences;
    std::vector<std::vector<std::string>> tokenized_texts;
    
    // Tokenize all texts
    for (const std::string& text : texts) {
        tokenized_texts.push_back(tokenizeForComparison(text));
        differences.push_back(std::vector<bool>());
    }
    
    // Helper method to extract clean word from display word
    auto getCleanWord = [](const std::string& display_word) -> std::string {
        std::string clean;
        for (char c : display_word) {
            if (std::isalnum(c) || c == '\'') {
                clean += std::tolower(c);
            }
        }
        return clean;
    };
    
    // Improved word-by-word comparison with better alignment
    for (size_t i = 0; i < tokenized_texts.size(); ++i) {
        const auto& tokens_i = tokenized_texts[i];
        auto& diffs_i = differences[i];
        diffs_i.resize(tokens_i.size(), false);
        
        for (size_t word_idx = 0; word_idx < tokens_i.size(); ++word_idx) {
            const std::string& display_word = tokens_i[word_idx];
            const std::string clean_word = getCleanWord(display_word);
            
            if (clean_word.empty()) {
                continue; // Skip punctuation-only tokens
            }
            
            // Check if this word appears in all other translations
            bool is_unique = true;
            int match_count = 0;
            
            for (size_t j = 0; j < tokenized_texts.size(); ++j) {
                if (i == j) continue;
                
                const auto& tokens_j = tokenized_texts[j];
                
                // Search for exact match first
                bool found_exact = false;
                for (const auto& other_display_word : tokens_j) {
                    const std::string other_clean = getCleanWord(other_display_word);
                    if (other_clean == clean_word) {
                        found_exact = true;
                        match_count++;
                        break;
                    }
                }
                
                // If no exact match, check for similar words (different forms)
                if (!found_exact) {
                    for (const auto& other_display_word : tokens_j) {
                        const std::string other_clean = getCleanWord(other_display_word);
                        // Simple similarity check for variations (e.g., "believes" vs "believeth")
                        if (clean_word.length() > 3 && other_clean.length() > 3) {
                            std::string word_root = clean_word.substr(0, std::min(clean_word.length() - 2, size_t(6)));
                            std::string other_root = other_clean.substr(0, std::min(other_clean.length() - 2, size_t(6)));
                            if (word_root == other_root) {
                                match_count++;
                                found_exact = true;
                                break;
                            }
                        }
                    }
                }
                
                if (!found_exact) {
                    is_unique = false;
                }
            }
            
            // Mark as different if it's unique to this translation or appears in less than half
            diffs_i[word_idx] = is_unique || (match_count < static_cast<int>((tokenized_texts.size() - 1) / 2));
        }
    }
    
    return differences;
}

void TranslationComparison::renderTranslationPanel(const std::string& translation, 
                                                  const std::string& text, 
                                                  const std::vector<bool>& word_diffs, 
                                                  int /* panel_index */) {
    // Translation header
    ImGui::Text("%s", translation.c_str());
    
    if (show_metadata) {
        renderMetadataInfo(translation);
    }
    
    ImGui::Separator();
    
    // Render text with highlighting
    if (show_word_differences && !word_diffs.empty()) {
        std::vector<std::string> words = tokenizeForComparison(text);
        
        ImGui::BeginGroup();
        for (size_t i = 0; i < words.size() && i < word_diffs.size(); ++i) {
            if (i > 0) ImGui::SameLine(0, 0);
            
            if (word_diffs[i]) {
                // Highlight different words with a subtle background
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.8f, 0.8f, 0.3f));
                ImGui::SmallButton(words[i].c_str());
                ImGui::PopStyleColor(2);
            } else {
                ImGui::Text("%s", words[i].c_str());
            }
        }
        ImGui::EndGroup();
    } else {
        ImGui::TextWrapped("%s", text.c_str());
    }
}

void TranslationComparison::renderMetadataInfo(const std::string& translation) {
    const auto& translations = verse_finder->getTranslations();
    for (const auto& trans_info : translations) {
        if (trans_info.name == translation) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
            if (!trans_info.description.empty()) {
                ImGui::Text("%s", trans_info.description.c_str());
            }
            if (trans_info.year > 0) {
                ImGui::Text("Year: %d", trans_info.year);
            }
            if (!trans_info.language.empty()) {
                ImGui::Text("Language: %s", trans_info.language.c_str());
            }
            ImGui::PopStyleColor();
            break;
        }
    }
}

std::vector<std::string> TranslationComparison::tokenizeForComparison(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream stream(text);
    std::string word;
    
    // Keep track of position for proper spacing
    bool first_word = true;
    
    while (stream >> word) {
        // Remove punctuation for comparison, but keep original for display
        std::string clean_word;
        std::string display_word = word;
        
        for (char c : word) {
            if (std::isalnum(c) || c == '\'') {
                clean_word += std::tolower(c);
            }
        }
        
        if (!clean_word.empty()) {
            // Store the display word with proper spacing
            if (!first_word) {
                display_word = " " + display_word;
            }
            tokens.push_back(display_word);
            first_word = false;
        }
    }
    
    return tokens;
}