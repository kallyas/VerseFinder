#ifndef RESULTS_COMPONENT_H
#define RESULTS_COMPONENT_H

#include <vector>
#include <string>
#include <functional>
#include "../../core/VerseFinder.h"

class ResultsComponent {
public:
    ResultsComponent(VerseFinder* verse_finder);
    ~ResultsComponent();

    void render(const std::vector<std::string>& results);
    
    // Result selection callbacks
    void setOnResultSelected(std::function<void(const std::string&)> callback) {
        on_result_selected = callback;
    }
    
    void setOnVerseClicked(std::function<void(const std::string&)> callback) {
        on_verse_clicked = callback;
    }
    
    // Display modes
    void setShowChapterView(bool show) { show_chapter_view = show; }
    bool getShowChapterView() const { return show_chapter_view; }
    
    // Navigation
    void navigateToVerse(int chapter, int verse);
    void jumpToVerse(const std::string& reference);
    
private:
    VerseFinder* verse_finder;
    
    // Callbacks
    std::function<void(const std::string&)> on_result_selected;
    std::function<void(const std::string&)> on_verse_clicked;
    
    // UI state
    bool show_chapter_view;
    std::string selected_result;
    int scroll_to_verse;
    
    // Rendering methods
    void renderEmptyState();
    void renderResultsList(const std::vector<std::string>& results);
    void renderChapterView(const std::vector<std::string>& results);
    void renderResultItem(const std::string& result, int index);
    void renderVerseItem(const std::string& verse, int index);
    
    // Utility methods
    bool isChapterResult(const std::string& result);
    std::string extractBookChapter(const std::string& result);
    void copyToClipboard(const std::string& text);
};

#endif // RESULTS_COMPONENT_H