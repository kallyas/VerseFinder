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
        
        // Reference input
        char ref_buffer[256];
        strncpy(ref_buffer, current_reference.c_str(), sizeof(ref_buffer) - 1);
        ref_buffer[sizeof(ref_buffer) - 1] = '\0';
        
        if (ImGui::InputText("Reference", ref_buffer, sizeof(ref_buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
            current_reference = std::string(ref_buffer);
            updateComparison();
            if (on_reference_changed) {
                on_reference_changed(current_reference);
            }
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
    
    // Simple word-by-word comparison
    // Mark words as different if they don't appear in all other texts
    for (size_t i = 0; i < tokenized_texts.size(); ++i) {
        const auto& tokens_i = tokenized_texts[i];
        auto& diffs_i = differences[i];
        diffs_i.resize(tokens_i.size(), false);
        
        for (size_t word_idx = 0; word_idx < tokens_i.size(); ++word_idx) {
            const std::string& word = tokens_i[word_idx];
            
            // Check if this word appears in all other translations at similar positions
            bool is_different = false;
            for (size_t j = 0; j < tokenized_texts.size(); ++j) {
                if (i == j) continue;
                
                const auto& tokens_j = tokenized_texts[j];
                // Simple heuristic: check same position and nearby positions
                bool found = false;
                int search_range = 2;
                for (int offset = -search_range; offset <= search_range && !found; ++offset) {
                    int pos = static_cast<int>(word_idx) + offset;
                    if (pos >= 0 && pos < static_cast<int>(tokens_j.size())) {
                        if (tokens_j[pos] == word) {
                            found = true;
                        }
                    }
                }
                if (!found) {
                    is_different = true;
                    break;
                }
            }
            diffs_i[word_idx] = is_different;
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
        
        for (size_t i = 0; i < words.size() && i < word_diffs.size(); ++i) {
            if (i > 0) ImGui::SameLine(0, 0);
            
            if (word_diffs[i]) {
                // Highlight different words
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.6f, 1.0f));
                ImGui::Text("%s", words[i].c_str());
                ImGui::PopStyleColor();
            } else {
                ImGui::Text("%s", words[i].c_str());
            }
        }
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
    
    while (stream >> word) {
        // Remove punctuation for comparison
        std::string clean_word;
        for (char c : word) {
            if (std::isalnum(c) || c == '\'') {
                clean_word += std::tolower(c);
            }
        }
        if (!clean_word.empty()) {
            tokens.push_back(clean_word);
        }
        
        // Also add the original word with spacing for display
        if (tokens.size() > 1) {
            tokens.back() = word + " ";
        } else {
            tokens.back() = word + " ";
        }
    }
    
    return tokens;
}