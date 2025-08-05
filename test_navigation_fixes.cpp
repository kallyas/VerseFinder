#include "src/core/VerseFinder.h"
#include <iostream>

void testNavigationFixes() {
    VerseFinder bible;
    bible.startLoading("sample_bible.json");
    
    // Wait for bible to load
    while (!bible.isReady()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "=== Testing Navigation Fixes ===" << std::endl;
    
    // Test scenarios that should work
    std::cout << "\n--- Testing valid navigation scenarios ---" << std::endl;
    
    // Test forward navigation within chapter
    std::string result = bible.getAdjacentVerse("John 3:16", "Sample", 1);
    std::cout << "John 3:16 +1: " << (result.empty() ? "FAILED" : "SUCCESS - " + result) << std::endl;
    
    // Test backward navigation within chapter
    result = bible.getAdjacentVerse("John 3:17", "Sample", -1);
    std::cout << "John 3:17 -1: " << (result.empty() ? "FAILED" : "SUCCESS - " + result) << std::endl;
    
    // Test navigation between chapters
    result = bible.getAdjacentVerse("Genesis 1:2", "Sample", -1);
    std::cout << "Genesis 1:2 -1: " << (result.empty() ? "FAILED" : "SUCCESS - " + result) << std::endl;
    
    // Test multi-step navigation
    result = bible.getAdjacentVerse("Genesis 1:1", "Sample", 2);
    std::cout << "Genesis 1:1 +2: " << (result.empty() ? "FAILED" : "SUCCESS - " + result) << std::endl;
    
    // Test scenarios that should fail (edge cases)
    std::cout << "\n--- Testing edge case scenarios (should fail gracefully) ---" << std::endl;
    
    // Test backward from first verse (should fail)
    result = bible.getAdjacentVerse("Genesis 1:1", "Sample", -1);
    std::cout << "Genesis 1:1 -1 (boundary): " << (result.empty() ? "SUCCESS - correctly failed" : "FAILED - " + result) << std::endl;
    
    // Test forward from last available verse 
    result = bible.getAdjacentVerse("Romans 8:28", "Sample", 1);
    std::cout << "Romans 8:28 +1 (boundary): " << (result.empty() ? "SUCCESS - correctly failed" : "FAILED - " + result) << std::endl;
    
    // Test with invalid references
    result = bible.getAdjacentVerse("NonexistentBook 1:1", "Sample", 1);
    std::cout << "Invalid book: " << (result.empty() ? "SUCCESS - correctly failed" : "FAILED - " + result) << std::endl;
    
    result = bible.getAdjacentVerse("John 999:1", "Sample", 1);
    std::cout << "Invalid chapter: " << (result.empty() ? "SUCCESS - correctly failed" : "FAILED - " + result) << std::endl;
    
    // Test large navigation steps
    std::cout << "\n--- Testing large navigation steps ---" << std::endl;
    
    result = bible.getAdjacentVerse("Genesis 1:1", "Sample", 10);
    std::cout << "Genesis 1:1 +10: " << (result.empty() ? "FAILED" : "SUCCESS - " + result) << std::endl;
    
    result = bible.getAdjacentVerse("Romans 8:28", "Sample", -10);
    std::cout << "Romans 8:28 -10: " << (result.empty() ? "FAILED" : "SUCCESS - " + result) << std::endl;
    
    std::cout << "\n=== Navigation Tests Complete ===" << std::endl;
}

int main() {
    try {
        testNavigationFixes();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}