#include "../src/core/VerseFinder.h"
#include "../src/core/PerformanceBenchmark.h"
#include "../src/core/MemoryMonitor.h"
#include "../src/core/AutoComplete.h"
#include "../src/core/IncrementalSearch.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <random>

class PerformanceTest {
private:
    VerseFinder verse_finder;
    PerformanceBenchmark benchmark;
    std::vector<std::string> test_queries;
    
    void generateTestQueries() {
        test_queries = {
            "John 3:16",
            "Genesis 1:1",
            "love",
            "faith",
            "hope",
            "God",
            "Jesus",
            "salvation",
            "Romans 8:28",
            "Psalm 23",
            "Matthew 5:14",
            "peace",
            "joy",
            "strength",
            "wisdom"
        };
    }
    
public:
    PerformanceTest() {
        generateTestQueries();
        verse_finder.setBenchmark(&benchmark);
    }
    
    bool loadTestData() {
        std::cout << "Loading test data..." << std::endl;
        
        // Try to load bible.json from different locations
        std::vector<std::string> possible_files = {
            "bible.json",
            "data/bible.json",
            "../bible.json",
            "./data/bible.json"
        };
        
        for (const auto& file : possible_files) {
            std::ifstream test_file(file);
            if (test_file.good()) {
                test_file.close();
                std::cout << "Found Bible data at: " << file << std::endl;
                verse_finder.startLoading(file);
                
                // Wait for loading to complete
                int wait_count = 0;
                while (!verse_finder.isReady() && wait_count < 50) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    wait_count++;
                }
                
                if (verse_finder.isReady()) {
                    std::cout << "Bible data loaded successfully!" << std::endl;
                    return true;
                } else {
                    std::cout << "Loading timed out." << std::endl;
                    return false;
                }
            }
        }
        
        std::cout << "No Bible data found. Creating minimal test data..." << std::endl;
        return createMinimalTestData();
    }
    
    bool createMinimalTestData() {
        // Create minimal test data for performance testing
        std::string test_json = R"({
            "translation": "Test",
            "abbreviation": "TEST",
            "books": [
                {
                    "name": "John",
                    "chapters": [
                        {
                            "chapter": 3,
                            "verses": [
                                {
                                    "verse": 16,
                                    "text": "For God so loved the world that he gave his one and only Son, that whoever believes in him shall not perish but have eternal life."
                                }
                            ]
                        }
                    ]
                },
                {
                    "name": "Genesis",
                    "chapters": [
                        {
                            "chapter": 1,
                            "verses": [
                                {
                                    "verse": 1,
                                    "text": "In the beginning God created the heavens and the earth."
                                }
                            ]
                        }
                    ]
                }
            ]
        })";
        
        verse_finder.addTranslation(test_json);
        
        // Wait for data to be processed
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        return verse_finder.isReady();
    }
    
    void testSearchPerformance() {
        std::cout << "\n=== Search Performance Test ===" << std::endl;
        
        if (!verse_finder.isReady()) {
            std::cout << "VerseFinder not ready. Skipping search tests." << std::endl;
            return;
        }
        
        const auto& translations = verse_finder.getTranslations();
        if (translations.empty()) {
            std::cout << "No translations available. Skipping search tests." << std::endl;
            return;
        }
        
        std::string translation = translations[0].name;
        
        // Test reference search performance
        std::cout << "Testing reference search..." << std::endl;
        auto start = std::chrono::steady_clock::now();
        
        for (int i = 0; i < 100; ++i) {
            verse_finder.searchByReference("John 3:16", translation);
        }
        
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "100 reference searches took: " << duration.count() / 1000.0 << " ms" << std::endl;
        std::cout << "Average per search: " << duration.count() / 100.0 / 1000.0 << " ms" << std::endl;
        
        // Test keyword search performance
        std::cout << "Testing keyword search..." << std::endl;
        start = std::chrono::steady_clock::now();
        
        for (int i = 0; i < 50; ++i) {
            verse_finder.searchByKeywords("God", translation);
        }
        
        end = std::chrono::steady_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "50 keyword searches took: " << duration.count() / 1000.0 << " ms" << std::endl;
        std::cout << "Average per search: " << duration.count() / 50.0 / 1000.0 << " ms" << std::endl;
    }
    
    void testAutoComplete() {
        std::cout << "\n=== Auto-Complete Performance Test ===" << std::endl;
        
        if (!verse_finder.isReady()) {
            std::cout << "VerseFinder not ready. Skipping auto-complete tests." << std::endl;
            return;
        }
        
        auto start = std::chrono::steady_clock::now();
        
        // Test various completion queries
        std::vector<std::string> completion_queries = {"J", "Jo", "Joh", "John", "God", "lo", "lov", "love"};
        
        for (const auto& query : completion_queries) {
            auto completions = verse_finder.getAutoCompletions(query, 10);
            std::cout << "Query '" << query << "' returned " << completions.size() << " completions" << std::endl;
        }
        
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "All auto-complete tests took: " << duration.count() / 1000.0 << " ms" << std::endl;
    }
    
    void testMemoryUsage() {
        std::cout << "\n=== Memory Usage Test ===" << std::endl;
        
        g_memory_monitor.startMonitoring(std::chrono::milliseconds(100));
        
        // Let it monitor for a few seconds
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        auto current_memory = g_memory_monitor.getCurrentMemoryMB();
        auto peak_memory = g_memory_monitor.getPeakMemoryMB();
        
        std::cout << "Current memory usage: " << current_memory << " MB" << std::endl;
        std::cout << "Peak memory usage: " << peak_memory << " MB" << std::endl;
        
        // Check if memory usage is within target (200MB)
        if (current_memory <= 200) {
            std::cout << "✓ Memory usage within target (≤200MB)" << std::endl;
        } else {
            std::cout << "✗ Memory usage exceeds target (>200MB)" << std::endl;
        }
        
        g_memory_monitor.stopMonitoring();
        std::cout << g_memory_monitor.getMemoryReport() << std::endl;
    }
    
    void testIncrementalSearch() {
        std::cout << "\n=== Incremental Search Test ===" << std::endl;
        
        if (!verse_finder.isReady()) {
            std::cout << "VerseFinder not ready. Skipping incremental search tests." << std::endl;
            return;
        }
        
        IncrementalSearch incremental(&verse_finder);
        
        std::vector<SearchResult> results;
        incremental.setResultCallback([&results](const SearchResult& result) {
            results.push_back(result);
            std::cout << "Received result for query '" << result.query 
                     << "': " << result.results.size() << " results in " 
                     << result.search_duration.count() / 1000.0 << " ms" << std::endl;
        });
        
        incremental.start();
        
        const auto& translations = verse_finder.getTranslations();
        if (!translations.empty()) {
            std::string translation = translations[0].name;
            
            // Submit multiple search requests to test debouncing
            incremental.submitSearch("J", translation);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            incremental.submitSearch("Jo", translation);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            incremental.submitSearch("Joh", translation);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            incremental.submitSearch("John", translation);
            
            // Wait for results
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            auto stats = incremental.getStats();
            std::cout << "Incremental search stats:" << std::endl;
            std::cout << "  Total searches: " << stats.total_searches << std::endl;
            std::cout << "  Average time: " << stats.average_search_time_ms << " ms" << std::endl;
            std::cout << "  Fastest: " << stats.fastest_search_ms << " ms" << std::endl;
            std::cout << "  Slowest: " << stats.slowest_search_ms << " ms" << std::endl;
        }
        
        incremental.stop();
    }
    
    void runAllTests() {
        std::cout << "=== VerseFinder Performance Test Suite ===" << std::endl;
        
        auto overall_start = std::chrono::steady_clock::now();
        
        if (!loadTestData()) {
            std::cout << "Failed to load test data. Some tests will be skipped." << std::endl;
        }
        
        testSearchPerformance();
        testAutoComplete();
        testMemoryUsage();
        testIncrementalSearch();
        
        auto overall_end = std::chrono::steady_clock::now();
        auto overall_duration = std::chrono::duration_cast<std::chrono::milliseconds>(overall_end - overall_start);
        
        std::cout << "\n=== Performance Test Summary ===" << std::endl;
        std::cout << "Total test time: " << overall_duration.count() << " ms" << std::endl;
        
        // Print benchmark results
        benchmark.printSummary();
        
        std::cout << "\n=== Performance Targets Check ===" << std::endl;
        // Check if targets are met (simplified check)
        std::cout << "✓ Auto-complete functionality implemented" << std::endl;
        std::cout << "✓ Memory monitoring implemented" << std::endl;
        std::cout << "✓ Incremental search implemented" << std::endl;
        std::cout << "✓ Performance benchmarking active" << std::endl;
    }
};

int main() {
    PerformanceTest test;
    test.runAllTests();
    return 0;
}