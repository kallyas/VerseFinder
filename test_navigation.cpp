#include "src/core/VerseFinder.h"
#include <iostream>
#include <string>

void testNavigationLogic() {
    VerseFinder bible;
    bible.startLoading("sample_bible.json");
    
    // Wait for bible to load
    while (!bible.isReady()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "Bible loaded successfully!" << std::endl;
    
    // Test navigation scenarios
    std::vector<std::string> test_references = {
        "John 3:16",
        "1 John 1:1",
        "Psalm 23:1",
        "Genesis 1:1",
        "Revelation 22:21"
    };
    
    for (const auto& ref : test_references) {
        std::cout << "\n=== Testing navigation from: " << ref << " ===" << std::endl;
        
        // Get current verse
        std::string current_verse = bible.searchByReference(ref, "Sample");
        if (current_verse == "Verse not found.") {
            std::cout << "Current verse not found: " << ref << std::endl;
            continue;
        }
        
        std::cout << "Current: " << ref << std::endl;
        
        // Test next verse
        std::string next_verse = bible.getAdjacentVerse(ref, "Sample", 1);
        std::cout << "Next (+1): " << next_verse << std::endl;
        
        // Test previous verse
        std::string prev_verse = bible.getAdjacentVerse(ref, "Sample", -1);
        std::cout << "Prev (-1): " << prev_verse << std::endl;
        
        // Test edge cases
        if (ref == "Genesis 1:1") {
            std::cout << "Previous from Genesis 1:1 (edge case): " << prev_verse << std::endl;
        }
        
        if (ref == "Revelation 22:21") {
            std::cout << "Next from Revelation 22:21 (edge case): " << next_verse << std::endl;
        }
    }
    
    // Test the UI's formatVerseReference function logic
    std::cout << "\n=== Testing Reference Parsing ===" << std::endl;
    std::vector<std::string> verse_text_samples = {
        "John 3:16: For God so loved the world...",
        "1 John 1:1: That which was from the beginning...",
        "Genesis 1:1: In the beginning God created...",
        "Song of Songs 1:1: The song of songs...",
        "1 Chronicles 1:1: Adam, Sheth, Enosh...",
        "Invalid format without colon"
    };
    
    for (const auto& verse_text : verse_text_samples) {
        size_t colon_pos = verse_text.find(": ");
        std::string extracted_ref = (colon_pos != std::string::npos) ? verse_text.substr(0, colon_pos) : "";
        std::cout << "Verse text: " << verse_text.substr(0, 50) << "..." << std::endl;
        std::cout << "Extracted reference: '" << extracted_ref << "'" << std::endl;
        
        // Test the parsing logic used in getAdjacentVerse
        if (!extracted_ref.empty()) {
            size_t space_pos = extracted_ref.find_last_of(' ');
            if (space_pos != std::string::npos) {
                std::string book = extracted_ref.substr(0, space_pos);
                std::string chapter_verse = extracted_ref.substr(space_pos + 1);
                std::cout << "Book: '" << book << "', Chapter:Verse: '" << chapter_verse << "'" << std::endl;
                
                size_t ch_colon_pos = chapter_verse.find(':');
                if (ch_colon_pos != std::string::npos) {
                    std::string chapter = chapter_verse.substr(0, ch_colon_pos);
                    std::string verse = chapter_verse.substr(ch_colon_pos + 1);
                    std::cout << "Chapter: '" << chapter << "', Verse: '" << verse << "'" << std::endl;
                }
            }
        }
        std::cout << std::endl;
    }
}

int main() {
    try {
        testNavigationLogic();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}