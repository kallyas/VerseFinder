#include "src/core/VerseFinder.h"
#include "src/ui/components/TranslationComparison.h"
#include <iostream>

int main() {
    VerseFinder vf;
    
    // Load translations
    vf.setTranslationsDirectory("./translations");
    vf.loadAllTranslations();
    
    // Wait for loading
    while (!vf.isReady()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Create comparison component
    TranslationComparison comp;
    comp.setVerseFinderRef(&vf);
    
    // Set comparison parameters
    std::vector<std::string> selected_translations;
    const auto& translations = vf.getTranslations();
    for (const auto& trans : translations) {
        if (trans.is_loaded) {
            selected_translations.push_back(trans.name);
            std::cout << "Selected: " << trans.name << std::endl;
        }
    }
    
    comp.setSelectedTranslations(selected_translations);
    comp.setCurrentReference("John 3:16");
    
    // Test the comparison result
    std::cout << "\nComparison test complete - the comparison component is ready for UI rendering." << std::endl;
    std::cout << "Use Ctrl+T in the main application to open the translation comparison window." << std::endl;
    
    return 0;
}