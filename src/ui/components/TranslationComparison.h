#ifndef TRANSLATION_COMPARISON_H
#define TRANSLATION_COMPARISON_H

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include "../../core/VerseFinder.h"

struct ComparisonResult {
    std::string reference;
    std::vector<std::pair<std::string, std::string>> translation_texts; // {translation_name, text}
    std::vector<std::vector<bool>> word_differences; // per translation, per word
};

class TranslationComparison {
public:
    TranslationComparison();
    ~TranslationComparison();

    void render();
    
    // Configuration
    void setVerseFinderRef(VerseFinder* vf) { verse_finder = vf; }
    void setSelectedTranslations(const std::vector<std::string>& translations);
    void setCurrentReference(const std::string& reference);
    
    // Callbacks
    void setOnReferenceChanged(std::function<void(const std::string&)> callback) {
        on_reference_changed = callback;
    }

private:
    VerseFinder* verse_finder;
    std::vector<std::string> selected_translations;
    std::string current_reference;
    ComparisonResult current_comparison;
    
    // UI state
    bool show_word_differences = true;
    bool show_metadata = true;
    float comparison_height = 300.0f;
    
    // Callbacks
    std::function<void(const std::string&)> on_reference_changed;
    
    // Helper methods
    void updateComparison();
    ComparisonResult compareVerseTexts(const std::string& reference);
    std::vector<std::vector<bool>> analyzeWordDifferences(const std::vector<std::string>& texts);
    void renderTranslationPanel(const std::string& translation, const std::string& text, 
                               const std::vector<bool>& word_diffs, int panel_index);
    void renderMetadataInfo(const std::string& translation);
    std::vector<std::string> tokenizeForComparison(const std::string& text);
};

#endif // TRANSLATION_COMPARISON_H