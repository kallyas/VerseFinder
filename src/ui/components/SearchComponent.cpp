#include "SearchComponent.h"
#include <imgui.h>
#include <cstring>
#include <algorithm>

SearchComponent::SearchComponent(VerseFinder* verse_finder)
    : verse_finder(verse_finder)
    , show_autocomplete(false)
    , fuzzy_search_enabled(true)
    , incremental_search_enabled(true)
    , search_input_focused(false) {
    
    memset(search_input, 0, sizeof(search_input));
}

SearchComponent::~SearchComponent() {
}

void SearchComponent::render() {
    renderSearchInput();
    renderSearchHistory();
    renderSearchButton();
    renderAutoComplete();
    renderSearchHints();
}

void SearchComponent::renderSearchInput() {
    // Modern search header with better styling
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
    ImGui::Text("Bible Search");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Modern search input with enhanced styling
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.25f, 0.25f, 0.3f, 1.0f));
    ImGui::PushItemWidth(-1);
    
    bool search_changed = ImGui::InputTextWithHint("##search", "Enter verse reference (e.g., 'John 3:16') or keywords...", 
                                                  search_input, sizeof(search_input), ImGuiInputTextFlags_EnterReturnsTrue);
    
    ImGui::PopItemWidth();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
    
    search_input_focused = ImGui::IsItemActive();
    
    // Handle search triggers
    if (search_changed) {
        performSearch();
    } else if (incremental_search_enabled && strcmp(search_input, last_search_input.c_str()) != 0) {
        performIncrementalSearch();
    }
    
    last_search_input = search_input;
}

void SearchComponent::renderSearchButton() {
    // Modern button styling
    ImGui::Spacing();
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16.0f, 8.0f));
    
    // Primary search button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.4f, 0.8f, 1.0f));
    if (ImGui::Button("Search", ImVec2(100, 0))) {
        performSearch();
    }
    ImGui::PopStyleColor(3);
    
    ImGui::SameLine();
    
    // Secondary clear button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.6f, 0.4f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.7f, 0.7f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 0.8f));
    if (ImGui::Button("Clear", ImVec2(80, 0))) {
        clearSearchInput();
    }
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
    
    // Search options
    ImGui::SameLine();
    ImGui::Text("Auto: ");
    ImGui::SameLine();
    ImGui::Checkbox("##auto_search", &incremental_search_enabled);
    
    ImGui::SameLine();
    ImGui::Text("Fuzzy: ");
    ImGui::SameLine();
    ImGui::Checkbox("##fuzzy_search", &fuzzy_search_enabled);
}

void SearchComponent::renderSearchHistory() {
    // Search history dropdown (if history exists)
    if (!search_history.empty()) {
        ImGui::Spacing();
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.3f, 0.35f, 1.0f));
        if (ImGui::BeginCombo("Recent Searches", nullptr)) {
            for (const auto& historical_search : search_history) {
                if (ImGui::Selectable(historical_search.c_str())) {
                    setSearchInput(historical_search);
                    performSearch();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }
}

void SearchComponent::renderAutoComplete() {
    // Book name suggestions
    if (!book_suggestions.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "Did you mean:");
        for (size_t i = 0; i < std::min(book_suggestions.size(), size_t(3)); ++i) {
            const auto& suggestion = book_suggestions[i];
            std::string confidence_text = "";
            if (suggestion.matchType == "fuzzy") {
                confidence_text = " (~" + std::to_string(static_cast<int>(suggestion.confidence * 100)) + "%)";
            } else if (suggestion.matchType == "phonetic") {
                confidence_text = " (phonetic)";
            } else if (suggestion.matchType == "partial") {
                confidence_text = " (...)";
            }
            
            if (ImGui::SmallButton((suggestion.text + confidence_text).c_str())) {
                setSearchInput(suggestion.text);
                performSearch();
            }
            if (i < std::min(book_suggestions.size(), size_t(3)) - 1) {
                ImGui::SameLine();
            }
        }
    }
    
    // Query keyword suggestions
    if (!query_suggestions.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "Suggestions:");
        for (size_t i = 0; i < std::min(query_suggestions.size(), size_t(5)); ++i) {
            if (ImGui::SmallButton(query_suggestions[i].c_str())) {
                setSearchInput(query_suggestions[i]);
                performSearch();
            }
            if (i < std::min(query_suggestions.size(), size_t(5)) - 1) {
                ImGui::SameLine();
            }
        }
    }
}

void SearchComponent::renderSearchHints() {
    // Search hints
    if (strlen(search_input) == 0) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Examples:");
        ImGui::BulletText("John 3:16 - Find specific verse");
        ImGui::BulletText("love - Find verses with keyword");
        ImGui::BulletText("faith hope love - Find multiple keywords");
        ImGui::BulletText("Psalm 23 - Find chapter references");
        
        if (fuzzy_search_enabled) {
            renderFuzzySearchExamples();
        }
    }
}

void SearchComponent::renderFuzzySearchExamples() {
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "Fuzzy Search Examples:");
    ImGui::BulletText("Jhn 3:16 - Corrects typos in book names");
    ImGui::BulletText("luv - Finds 'love' with phonetic matching");
    ImGui::BulletText("Gen - Suggests 'Genesis' from partial match");
    ImGui::BulletText("fait - Suggests 'faith' from similar spelling");
}

void SearchComponent::setSearchInput(const std::string& input) {
    strncpy(search_input, input.c_str(), sizeof(search_input) - 1);
    search_input[sizeof(search_input) - 1] = '\0';
}

void SearchComponent::clearSearchInput() {
    memset(search_input, 0, sizeof(search_input));
    search_results.clear();
    book_suggestions.clear();
    query_suggestions.clear();
}

void SearchComponent::performSearch() {
    if (!verse_finder || strlen(search_input) == 0) {
        search_results.clear();
        return;
    }
    
    std::string query = search_input;
    search_results = verse_finder->searchByKeywords(query, "KJV"); // Default translation
    
    // Add to search history
    addToSearchHistory(query);
    
    // Update suggestions based on results
    updateAutoComplete();
}

void SearchComponent::performIncrementalSearch() {
    if (!verse_finder || strlen(search_input) < 2) {
        return;
    }
    
    // Only update autocomplete for incremental search
    updateAutoComplete();
}

void SearchComponent::updateAutoComplete() {
    if (!verse_finder) return;
    
    std::string query = search_input;
    if (query.empty()) {
        book_suggestions.clear();
        query_suggestions.clear();
        return;
    }
    
    // Get book suggestions if fuzzy search is enabled
    if (fuzzy_search_enabled) {
        book_suggestions = verse_finder->findBookNameSuggestions(query);
    }
    
    // Get query suggestions (simplified - would normally use more sophisticated logic)
    query_suggestions.clear();
    if (query.length() >= 2) {
        // This would typically call a method to get keyword suggestions
        // For now, we'll leave it empty as the full implementation would require
        // access to the verse finder's internal methods
    }
}

void SearchComponent::selectAutoComplete(int index) {
    if (index >= 0 && index < static_cast<int>(book_suggestions.size())) {
        setSearchInput(book_suggestions[index].text);
        performSearch();
    }
}

void SearchComponent::addToSearchHistory(const std::string& query) {
    // Remove duplicates
    auto it = std::find(search_history.begin(), search_history.end(), query);
    if (it != search_history.end()) {
        search_history.erase(it);
    }
    
    // Add to front
    search_history.insert(search_history.begin(), query);
    
    // Limit history size
    if (search_history.size() > 10) {
        search_history.resize(10);
    }
}