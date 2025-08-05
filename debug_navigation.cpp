#include "src/core/VerseFinder.h"
#include <iostream>
#include <string>

void debugNavigationIssue() {
    VerseFinder bible;
    bible.startLoading("sample_bible.json");
    
    // Wait for bible to load
    while (!bible.isReady()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "=== Debug Previous Verse Navigation Issue ===" << std::endl;
    
    // Test with John 3:16 going back (should find nothing before verse 16)
    std::string reference = "John 3:16";
    std::string translation = "Sample";
    int direction = -1;
    
    std::cout << "Testing: " << reference << " with direction " << direction << std::endl;
    
    // Parse the reference
    size_t space_pos = reference.find_last_of(' ');
    std::string book = reference.substr(0, space_pos);
    std::string chapter_verse = reference.substr(space_pos + 1);
    
    size_t colon_pos = chapter_verse.find(':');
    int chapter = std::stoi(chapter_verse.substr(0, colon_pos));
    int verse = std::stoi(chapter_verse.substr(colon_pos + 1));
    
    std::cout << "Parsed - Book: '" << book << "', Chapter: " << chapter << ", Verse: " << verse << std::endl;
    
    // Check current verse exists
    bool current_exists = bible.verseExists(book, chapter, verse, translation);
    std::cout << "Current verse exists: " << current_exists << std::endl;
    
    // Test the navigation logic step by step
    int current_chapter = chapter;
    int current_verse = verse;
    int steps_remaining = std::abs(direction);
    int step_direction = (direction > 0) ? 1 : -1;
    
    std::cout << "Navigation parameters: current_chapter=" << current_chapter << ", current_verse=" << current_verse 
              << ", steps_remaining=" << steps_remaining << ", step_direction=" << step_direction << std::endl;
    
    for (int i = 0; i < steps_remaining; i++) {
        std::cout << "Step " << i << ":" << std::endl;
        int next_verse = current_verse + step_direction;
        std::cout << "  Calculating next_verse: " << current_verse << " + " << step_direction << " = " << next_verse << std::endl;
        
        if (step_direction > 0) {
            // Moving forward
            std::cout << "  Moving forward" << std::endl;
            if (!bible.verseExists(book, current_chapter, next_verse, translation)) {
                std::cout << "  Next verse doesn't exist, trying next chapter" << std::endl;
                current_chapter++;
                next_verse = 1;
                if (!bible.verseExists(book, current_chapter, next_verse, translation)) {
                    std::cout << "  Next chapter doesn't exist either" << std::endl;
                    if (i == 0) return; // Couldn't move at all
                    break; // Stop here with current position
                }
            }
        } else {
            // Moving backward
            std::cout << "  Moving backward" << std::endl;
            if (next_verse < 1) {
                std::cout << "  next_verse < 1, going to previous chapter" << std::endl;
                current_chapter--;
                if (current_chapter < 1) {
                    std::cout << "  current_chapter < 1, reached beginning of book" << std::endl;
                    if (i == 0) return; // Couldn't move at all
                    break; // Stop here with current position
                }
                next_verse = bible.getLastVerseInChapter(book, current_chapter, translation);
                std::cout << "  Last verse in previous chapter: " << next_verse << std::endl;
                if (next_verse == 0) {
                    std::cout << "  Chapter doesn't exist" << std::endl;
                    if (i == 0) return; // Couldn't move at all
                    break; // Stop here with current position
                }
            } else {
                std::cout << "  Checking if verse " << next_verse << " exists in chapter " << current_chapter << std::endl;
                bool verse_exists = bible.verseExists(book, current_chapter, next_verse, translation);
                std::cout << "  Verse exists: " << verse_exists << std::endl;
            }
        }
        
        current_verse = next_verse;
        std::cout << "  Updated current_verse to: " << current_verse << std::endl;
    }
    
    // Construct new reference and search for it
    std::string new_reference = book + " " + std::to_string(current_chapter) + ":" + std::to_string(current_verse);
    std::cout << "Final reference: " << new_reference << std::endl;
    std::string result = bible.searchByReference(new_reference, translation);
    std::cout << "Search result: " << result << std::endl;
    
    if (result != "Verse not found.") {
        std::string full_result = new_reference + ": " + result;
        std::cout << "Full result: " << full_result << std::endl;
    } else {
        std::cout << "Result is empty" << std::endl;
    }
}

int main() {
    try {
        debugNavigationIssue();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}