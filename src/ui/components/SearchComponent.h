#ifndef SEARCH_COMPONENT_H
#define SEARCH_COMPONENT_H

#include <string>
#include <vector>
#include <functional>
#include "../../core/VerseFinder.h"
#include "../../core/FuzzySearch.h"

class SearchComponent {
public:
    SearchComponent(VerseFinder* verse_finder);
    ~SearchComponent();

    void render();
    
    // Search input management
    const char* getSearchInput() const { return search_input; }
    void setSearchInput(const std::string& input);
    void clearSearchInput();
    
    // Search execution
    void performSearch();
    bool hasResults() const { return !search_results.empty(); }
    const std::vector<std::string>& getResults() const { return search_results; }
    
    // Auto-complete functionality
    void updateAutoComplete();
    void selectAutoComplete(int index);
    
    // Search settings
    void setFuzzySearchEnabled(bool enabled) { fuzzy_search_enabled = enabled; }
    bool isFuzzySearchEnabled() const { return fuzzy_search_enabled; }
    
    void setIncrementalSearchEnabled(bool enabled) { incremental_search_enabled = enabled; }
    bool isIncrementalSearchEnabled() const { return incremental_search_enabled; }
    
    // Advanced search features
    void showTopicSuggestions();
    void showRelatedQueries(const std::string& query);
    void showSeasonalSuggestions();
    
    // Callback setters
    void setOnResultSelected(std::function<void(const std::string&)> callback) { 
        on_result_selected = callback; 
    }

private:
    VerseFinder* verse_finder;
    
    char search_input[512];
    std::vector<std::string> search_results;
    std::vector<std::string> search_history;
    
    // Auto-complete
    std::vector<FuzzyMatch> book_suggestions;
    std::vector<std::string> query_suggestions;
    
    // Search settings
    bool fuzzy_search_enabled;
    bool incremental_search_enabled;
    
    // Advanced search features
    std::vector<std::string> topic_suggestions;
    std::vector<std::string> related_queries;
    std::vector<std::string> seasonal_suggestions;
    bool show_advanced_suggestions;
    
    // UI state
    bool search_input_focused;
    std::string last_search_input;
    
    // Callbacks
    std::function<void(const std::string&)> on_result_selected;
    
    void renderSearchInput();
    void renderSearchButton();
    void renderSearchHistory();
    void renderAutoComplete();
    void renderAdvancedSuggestions();
    void renderSearchHints();
    void renderFuzzySearchExamples();
    
    void performIncrementalSearch();
    void addToSearchHistory(const std::string& query);
};

#endif // SEARCH_COMPONENT_H