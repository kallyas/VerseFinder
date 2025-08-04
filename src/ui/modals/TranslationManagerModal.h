#ifndef TRANSLATION_MANAGER_MODAL_H
#define TRANSLATION_MANAGER_MODAL_H

#include <string>
#include <vector>
#include <functional>
#include "../../core/VerseFinder.h"

class TranslationManagerModal {
public:
    TranslationManagerModal();
    ~TranslationManagerModal();

    void render();
    
    // Configuration
    void setVerseFinderRef(VerseFinder* vf) { verse_finder = vf; }
    void setVisible(bool visible) { show_modal = visible; }
    bool isVisible() const { return show_modal; }
    
    // Callbacks
    void setOnTranslationChanged(std::function<void()> callback) {
        on_translation_changed = callback;
    }

private:
    VerseFinder* verse_finder;
    bool show_modal = false;
    
    // UI state
    std::string new_translation_path;
    char path_buffer[512] = "";
    bool show_file_browser = false;
    
    // Callbacks
    std::function<void()> on_translation_changed;
    
    // Helper methods
    void renderTranslationList();
    void renderAddTranslation();
    void renderFilePathInput();
    bool loadTranslationFromFile(const std::string& file_path);
    void refreshTranslationList();
};

#endif // TRANSLATION_MANAGER_MODAL_H