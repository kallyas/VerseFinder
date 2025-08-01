#ifndef TRANSLATION_SELECTOR_H
#define TRANSLATION_SELECTOR_H

#include <string>
#include <vector>
#include <functional>
#include "../common/TranslationManager.h"

class TranslationSelector {
public:
    TranslationSelector();
    ~TranslationSelector();

    void render();
    
    // Translation management
    void setAvailableTranslations(const std::vector<DownloadableTranslation>& translations);
    void setCurrentTranslation(const std::string& translation_name);
    std::string getCurrentTranslation() const { return current_translation; }
    
    // Callbacks
    void setOnTranslationChanged(std::function<void(const std::string&)> callback) {
        on_translation_changed = callback;
    }
    
    void setOnManageTranslations(std::function<void()> callback) {
        on_manage_translations = callback;
    }

private:
    std::vector<DownloadableTranslation> available_translations;
    std::string current_translation;
    
    // Callbacks
    std::function<void(const std::string&)> on_translation_changed;
    std::function<void()> on_manage_translations;
    
    void initializeDefaultTranslations();
};

#endif // TRANSLATION_SELECTOR_H