#include "src/core/VerseFinder.h"
#include <iostream>

int main() {
    VerseFinder vf;
    
    // Test loading multiple translations
    std::cout << "Setting translations directory..." << std::endl;
    vf.setTranslationsDirectory("./translations");
    
    std::cout << "Loading all translations..." << std::endl;
    vf.loadAllTranslations();
    
    // Wait for loading to complete
    while (!vf.isReady()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    const auto& translations = vf.getTranslations();
    std::cout << "Loaded " << translations.size() << " translations:" << std::endl;
    
    for (const auto& trans : translations) {
        std::cout << "- " << trans.abbreviation << ": " << trans.name;
        if (!trans.description.empty()) {
            std::cout << " (" << trans.description << ")";
        }
        if (trans.year > 0) {
            std::cout << " [" << trans.year << "]";
        }
        std::cout << " - Loaded: " << (trans.is_loaded ? "Yes" : "No");
        std::cout << std::endl;
    }
    
    // Test search in different translations
    std::string verse_ref = "John 3:16";
    std::cout << "\nSearching for " << verse_ref << " in different translations:" << std::endl;
    
    for (const auto& trans : translations) {
        if (trans.is_loaded) {
            std::string result = vf.searchByReference(verse_ref, trans.name);
            std::cout << trans.abbreviation << ": " << result << std::endl;
        }
    }
    
    return 0;
}