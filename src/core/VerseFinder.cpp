
#include "VerseFinder.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <filesystem>
#include <set>
#include <unordered_set>
#include <future>
#include <mutex>

VerseFinder::VerseFinder() : benchmark(&g_benchmark) {
    book_aliases = {
        {"St. John", "John"},
        {"Saint John", "John"},
        {"1st John", "1 John"},
    };
}

void VerseFinder::startLoading(const std::string& filename) {
    loading_future = std::async(std::launch::async, &VerseFinder::loadBibleInternal, this, filename);
}

bool VerseFinder::isReady() const {
    return data_loaded;
}

void VerseFinder::loadBibleInternal(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filename << std::endl;
        return;
    }

    json j;
    file >> j;

    // Assuming a single translation at the top level for KJV data
    std::string trans_name = j["translation"];
    std::string trans_abbr = j["abbreviation"];
    available_translations.push_back({trans_name, trans_abbr});

    for (const auto& book_json : j["books"]) {
        std::string book_name = book_json["name"];
        for (const auto& chapter_json : book_json["chapters"]) {
            int chapter_num = chapter_json["chapter"];
            for (const auto& verse_json : chapter_json["verses"]) {
                Verse v{
                    book_name,
                    chapter_num,
                    verse_json["verse"],
                    verse_json["text"]
                };
                std::string key = makeKey(v.book, v.chapter, v.verse);
                verses[trans_name][key] = v;
                // Use optimized tokenization
                for (const auto& token : SearchOptimizer::optimizedTokenize(v.text)) {
                    keyword_index[trans_name][token].push_back(key);
                }
            }
        }
    }
    data_loaded = true;
    std::cout << "Loaded " << available_translations.size() << " translations." << std::endl;
}

std::string VerseFinder::normalizeBookName(const std::string& book) const {
    // First try exact match
    auto it = book_aliases.find(book);
    if (it != book_aliases.end()) {
        return it->second;
    }
    
    // Try case-insensitive match for book aliases
    std::string lower_book = book;
    std::transform(lower_book.begin(), lower_book.end(), lower_book.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    
    for (const auto& alias_pair : book_aliases) {
        std::string lower_alias = alias_pair.first;
        std::transform(lower_alias.begin(), lower_alias.end(), lower_alias.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        if (lower_alias == lower_book) {
            return alias_pair.second;
        }
    }
    
    // If no alias match, try case-insensitive match against actual book names in loaded data
    if (!verses.empty()) {
        const auto& first_translation = verses.begin()->second;
        
        // Collect unique book names from the loaded data
        std::set<std::string> book_names;
        for (const auto& verse_pair : first_translation) {
            const Verse& v = verse_pair.second;
            book_names.insert(v.book);
        }
        
        // Try case-insensitive match against actual book names
        for (const std::string& actual_book : book_names) {
            std::string lower_actual = actual_book;
            std::transform(lower_actual.begin(), lower_actual.end(), lower_actual.begin(),
                           [](unsigned char c){ return std::tolower(c); });
            if (lower_actual == lower_book) {
                return actual_book; // Return the correctly cased book name
            }
        }
    }
    
    // If no match found, return the original book name
    return book;
}

std::string VerseFinder::normalizeReference(const std::string& reference) const {
    // Parse the reference to extract book, chapter, and verse
    size_t space_pos = reference.find_last_of(' ');
    if (space_pos == std::string::npos) return reference;
    
    std::string book_part = reference.substr(0, space_pos);
    std::string chapter_verse = reference.substr(space_pos + 1);
    
    size_t colon_pos = chapter_verse.find(':');
    if (colon_pos == std::string::npos) return reference;
    
    try {
        int chapter = std::stoi(chapter_verse.substr(0, colon_pos));
        int verse = std::stoi(chapter_verse.substr(colon_pos + 1));
        
        // Normalize the book name and reconstruct the reference
        std::string normalized_book = normalizeBookName(book_part);
        return normalized_book + " " + std::to_string(chapter) + ":" + std::to_string(verse);
    } catch (const std::exception&) {
        // If parsing fails, return the original reference
        return reference;
    }
}

std::string VerseFinder::makeKey(const std::string& book, int chapter, int verse) const {
    return normalizeBookName(book) + " " + std::to_string(chapter) + ":" + std::to_string(verse);
}

std::vector<std::string> VerseFinder::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::string token;
    for (char c : text) {
        if (std::isalnum(c)) {
            token += std::tolower(c);
        } else if (!token.empty()) {
            tokens.push_back(token);
            token.clear();
        }
    }
    if (!token.empty()) tokens.push_back(token);
    return tokens;
}

std::string VerseFinder::searchByReference(const std::string& reference, const std::string& translation) const {
    if (!isReady()) return "Bible is loading...";
    
    BENCHMARK_SCOPE("reference_search");
    
    // Normalize the reference for case-insensitive lookup
    std::string normalized_ref = normalizeReference(reference);
    
    auto it = verses.find(translation);
    if (it != verses.end()) {
        const auto& inner = it->second;
        if (const auto verse_it = inner.find(normalized_ref); verse_it != inner.end()) {
            return verse_it->second.text;
        }
    }
    return "Verse not found.";
}

std::vector<std::string> VerseFinder::searchByKeywords(const std::string& query, const std::string& translation) const {
    if (!isReady()) return {"Bible is loading..."};
    
    // Check cache first
    std::vector<std::string> cached_results;
    if (search_cache.get(query, translation, cached_results)) {
        return cached_results;
    }
    
    // Use full text search instead of keyword intersection for better results
    std::vector<std::string> results = searchByFullText(query, translation);
    
    // Cache the results
    search_cache.put(query, translation, results);
    
    return results;
}

std::vector<std::string> VerseFinder::searchByKeywordsOptimized(const std::string& query, const std::string& translation) const {
    BENCHMARK_SCOPE("keyword_search");
    
    auto tokens = SearchOptimizer::optimizedTokenize(query);
    if (tokens.empty()) return {"No keywords provided."};

    auto trans_it = keyword_index.find(translation);
    if (trans_it == keyword_index.end()) {
        return {"Translation not found."};
    }
    const auto& index = trans_it->second;

    // Collect token lists for intersection
    std::vector<std::vector<std::string>> token_lists;
    token_lists.reserve(tokens.size());
    
    for (const auto& token : tokens) {
        auto token_it = index.find(token);
        if (token_it == index.end()) {
            return {"No matching verses found."};
        }
        token_lists.push_back(token_it->second);
    }

    // Use optimized intersection algorithm
    std::vector<std::string> common_refs = SearchOptimizer::optimizedIntersection(token_lists);

    if (common_refs.empty()) {
        return {"No matching verses found."};
    }

    // Verify phrase matches using optimized verification
    std::vector<std::string> results;
    results.reserve(common_refs.size());

    for (const auto& ref : common_refs) {
        const auto& verse_text = verses.at(translation).at(ref).text;
        
        if (SearchOptimizer::verifyPhraseMatch(verse_text, query)) {
            results.push_back(ref + ": " + verse_text);
        }
    }
    
    return results.empty() ? std::vector<std::string>{"No matching verses found."} : results;
}

std::vector<std::string> VerseFinder::searchByFullText(const std::string& query, const std::string& translation) const {
    if (!isReady()) return {"Bible is loading..."};
    
    BENCHMARK_SCOPE("full_text_search");
    
    if (query.empty()) return {"No search query provided."};
    
    // Convert query to lowercase for case-insensitive search
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    
    auto trans_it = verses.find(translation);
    if (trans_it == verses.end()) {
        return {"Translation not found."};
    }
    
    std::vector<std::string> results;
    const auto& translation_verses = trans_it->second;
    
    // Search through all verses in the translation
    for (const auto& verse_pair : translation_verses) {
        const std::string& verse_key = verse_pair.first;
        const Verse& verse = verse_pair.second;
        
        // Convert verse text to lowercase for comparison
        std::string lower_verse_text = verse.text;
        std::transform(lower_verse_text.begin(), lower_verse_text.end(), lower_verse_text.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        
        // Check if the query appears as a substring in the verse
        if (lower_verse_text.find(lower_query) != std::string::npos) {
            results.push_back(verse_key + ": " + verse.text);
        }
        // If not exact substring match, check if all query words appear in the verse
        else {
            auto query_words = tokenize(lower_query);
            bool all_words_found = true;
            
            for (const auto& word : query_words) {
                if (lower_verse_text.find(word) == std::string::npos) {
                    all_words_found = false;
                    break;
                }
            }
            
            if (all_words_found && !query_words.empty()) {
                results.push_back(verse_key + ": " + verse.text);
            }
        }
    }
    
    // Sort results by relevance (exact phrase matches first, then word matches)
    std::sort(results.begin(), results.end(), [&](const std::string& a, const std::string& b) {
        std::string text_a = a.substr(a.find(": ") + 2);
        std::string text_b = b.substr(b.find(": ") + 2);
        
        std::transform(text_a.begin(), text_a.end(), text_a.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        std::transform(text_b.begin(), text_b.end(), text_b.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        
        bool a_exact = text_a.find(lower_query) != std::string::npos;
        bool b_exact = text_b.find(lower_query) != std::string::npos;
        
        if (a_exact && !b_exact) return true;
        if (!a_exact && b_exact) return false;
        
        return false; // Keep original order for same relevance
    });
    
    return results.empty() ? std::vector<std::string>{"No matching verses found."} : results;
}

bool VerseFinder::parseReference(const std::string& reference, std::string& book, int& chapter, int& verse) const {
    // Initialize defaults
    book.clear();
    chapter = -1;
    verse = -1;
    
    // Find the last space to separate book from chapter:verse
    size_t space_pos = reference.find_last_of(' ');
    if (space_pos == std::string::npos) {
        // No space found - might be just a book name
        book = reference;
        return true;
    }
    
    book = reference.substr(0, space_pos);
    std::string chapter_verse = reference.substr(space_pos + 1);
    
    // Check if there's a colon for verse specification
    size_t colon_pos = chapter_verse.find(':');
    if (colon_pos != std::string::npos) {
        // Format: "Book Chapter:Verse"
        try {
            chapter = std::stoi(chapter_verse.substr(0, colon_pos));
            verse = std::stoi(chapter_verse.substr(colon_pos + 1));
            return true;
        } catch (const std::exception&) {
            return false;
        }
    } else {
        // Format: "Book Chapter" (chapter only)
        try {
            chapter = std::stoi(chapter_verse);
            return true;
        } catch (const std::exception&) {
            // Maybe it's part of the book name (e.g., "1 John")
            book = reference;
            return true;
        }
    }
}

std::vector<std::string> VerseFinder::searchByChapter(const std::string& reference, const std::string& translation) const {
    if (!isReady()) return {"Bible is loading..."};
    
    BENCHMARK_SCOPE("chapter_search");
    
    std::string book;
    int chapter, verse;
    
    if (!parseReference(reference, book, chapter, verse)) {
        return {"Invalid reference format."};
    }
    
    // Normalize the book name
    std::string normalized_book = normalizeBookName(book);
    
    auto trans_it = verses.find(translation);
    if (trans_it == verses.end()) {
        return {"Translation not found."};
    }
    
    std::vector<std::string> results;
    const auto& translation_verses = trans_it->second;
    
    // If only book is specified, return error message suggesting format
    if (chapter == -1) {
        return {"Please specify a chapter (e.g., \"" + book + " 1\") or verse (e.g., \"" + book + " 1:1\")."};
    }
    
    // Collect all verses from the specified chapter
    for (const auto& verse_pair : translation_verses) {
        const Verse& v = verse_pair.second;
        if (v.book == normalized_book && v.chapter == chapter) {
            std::string verse_ref = normalized_book + " " + std::to_string(v.chapter) + ":" + std::to_string(v.verse);
            results.push_back(verse_ref + ": " + v.text);
        }
    }
    
    // Sort by verse number
    std::sort(results.begin(), results.end(), [](const std::string& a, const std::string& b) {
        // Find the pattern "chapter:verse: " to extract verse numbers
        size_t colon_a = a.find(':');
        size_t colon_b = b.find(':');
        
        if (colon_a != std::string::npos && colon_b != std::string::npos) {
            size_t second_colon_a = a.find(':', colon_a + 1);
            size_t second_colon_b = b.find(':', colon_b + 1);
            
            if (second_colon_a != std::string::npos && second_colon_b != std::string::npos) {
                try {
                    // Extract verse number between first and second colon
                    std::string verse_str_a = a.substr(colon_a + 1, second_colon_a - colon_a - 1);
                    std::string verse_str_b = b.substr(colon_b + 1, second_colon_b - colon_b - 1);
                    
                    int verse_a = std::stoi(verse_str_a);
                    int verse_b = std::stoi(verse_str_b);
                    return verse_a < verse_b;
                } catch (const std::exception&) {
                    return false;
                }
            }
        }
        return false;
    });
    
    if (results.empty()) {
        return {"Chapter not found: " + normalized_book + " " + std::to_string(chapter)};
    }
    
    return results;
}

const std::vector<TranslationInfo>& VerseFinder::getTranslations() const {
    return available_translations;
}

void VerseFinder::addTranslation(const std::string& json_data) {
    json j = json::parse(json_data);

    std::string trans_name = j["translation"];
    std::string trans_abbr = j["abbreviation"];

    // Check if translation already exists
    for (const auto& trans_info : available_translations) {
        if (trans_info.name == trans_name) {
            std::cerr << "Translation " << trans_name << " already loaded." << std::endl;
            return;
        }
    }

    available_translations.push_back({trans_name, trans_abbr});

    for (const auto& book_json : j["books"]) {
        std::string book_name = book_json["name"];
        for (const auto& chapter_json : book_json["chapters"]) {
            int chapter_num = chapter_json["chapter"];
            for (const auto& verse_json : chapter_json["verses"]) {
                Verse v{
                    book_name,
                    chapter_num,
                    verse_json["verse"],
                    verse_json["text"]
                };
                std::string key = makeKey(v.book, v.chapter, v.verse);
                verses[trans_name][key] = v;
                // Use optimized tokenization
                for (const auto& token : SearchOptimizer::optimizedTokenize(v.text)) {
                    keyword_index[trans_name][token].push_back(key);
                }
            }
        }
    }
    std::cout << "Added translation: " << trans_name << std::endl;
}

void VerseFinder::setTranslationsDirectory(const std::string& dir_path) {
    translations_dir = dir_path;
    // Create translations directory if it doesn't exist
    if (!std::filesystem::exists(translations_dir)) {
        std::filesystem::create_directories(translations_dir);
        std::cout << "Created translations directory: " << translations_dir << std::endl;
    }
}

void VerseFinder::loadAllTranslations() {
    if (translations_dir.empty()) return;
    
    loading_future = std::async(std::launch::async, &VerseFinder::loadTranslationsFromDirectory, this, translations_dir);
}

void VerseFinder::loadTranslationsFromDirectory(const std::string& dir_path) {
    if (!std::filesystem::exists(dir_path)) {
        std::cout << "Translations directory does not exist: " << dir_path << std::endl;
        return;
    }
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Clear existing translations
    available_translations.clear();
    verses.clear();
    keyword_index.clear();
    
    // Collect all JSON files first
    std::vector<std::string> json_files;
    for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            json_files.push_back(entry.path().string());
        }
    }
    
    // If no translations found in directory, try to load bible.json from parent directory
    if (json_files.empty()) {
        std::string bible_file = dir_path + "/../bible.json";
        if (std::filesystem::exists(bible_file)) {
            json_files.push_back(bible_file);
        }
    }
    
    if (json_files.empty()) {
        std::cout << "No translation files found." << std::endl;
        return;
    }
    
    std::cout << "Loading " << json_files.size() << " translation files..." << std::endl;
    
    // Load files in parallel using async tasks
    std::vector<std::future<void>> loading_tasks;
    std::mutex data_mutex; // Protect shared data structures
    
    for (const auto& file_path : json_files) {
        loading_tasks.emplace_back(std::async(std::launch::async, [this, file_path, &data_mutex]() {
            loadSingleTranslationOptimized(file_path, data_mutex);
        }));
    }
    
    // Wait for all loading tasks to complete
    for (auto& task : loading_tasks) {
        task.wait();
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    data_loaded = true;
    std::cout << "Loaded " << available_translations.size() << " translations in " 
              << duration.count() << "ms." << std::endl;
}

void VerseFinder::loadSingleTranslation(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filename << std::endl;
        return;
    }

    json j;
    try {
        file >> j;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON file " << filename << ": " << e.what() << std::endl;
        return;
    }

    std::string trans_name = j.value("translation", "Unknown");
    std::string trans_abbr = j.value("abbreviation", "UNK");

    // Check if translation already exists
    for (const auto& trans_info : available_translations) {
        if (trans_info.name == trans_name) {
            std::cout << "Translation " << trans_name << " already loaded, skipping." << std::endl;
            return;
        }
    }

    available_translations.push_back({trans_name, trans_abbr});

    // Load verses
    if (j.contains("books") && j["books"].is_array()) {
        for (const auto& book_json : j["books"]) {
            std::string book_name = book_json.value("name", "Unknown Book");
            if (book_json.contains("chapters") && book_json["chapters"].is_array()) {
                for (const auto& chapter_json : book_json["chapters"]) {
                    int chapter_num = chapter_json.value("chapter", 1);
                    if (chapter_json.contains("verses") && chapter_json["verses"].is_array()) {
                        for (const auto& verse_json : chapter_json["verses"]) {
                            Verse v{
                                book_name,
                                chapter_num,
                                verse_json.value("verse", 1),
                                verse_json.value("text", "")
                            };
                            std::string key = makeKey(v.book, v.chapter, v.verse);
                            verses[trans_name][key] = v;
                            
                            // Build keyword index
                            for (const auto& token : tokenize(v.text)) {
                                keyword_index[trans_name][token].push_back(key);
                            }
                        }
                    }
                }
            }
        }
    }
    
    std::cout << "Successfully loaded translation: " << trans_name << " (" << trans_abbr << ")" << std::endl;
}

void VerseFinder::loadSingleTranslationOptimized(const std::string& filename, std::mutex& data_mutex) {
    // Load and parse JSON outside of the lock
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filename << std::endl;
        return;
    }

    json j;
    try {
        // Use more efficient JSON parsing
        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::string json_content;
        json_content.reserve(file_size); // Pre-allocate memory
        
        json_content.assign((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        
        j = json::parse(json_content);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON file " << filename << ": " << e.what() << std::endl;
        return;
    }

    std::string trans_name = j.value("translation", "Unknown");
    std::string trans_abbr = j.value("abbreviation", "UNK");

    // Pre-process all data structures without locks
    std::unordered_map<std::string, Verse> local_verses;
    std::unordered_map<std::string, std::vector<std::string>> local_keyword_index;
    
    // Estimate capacity for better performance
    if (j.contains("books") && j["books"].is_array()) {
        size_t estimated_verses = j["books"].size() * 25 * 30; // rough estimate
        local_verses.reserve(estimated_verses);
        local_keyword_index.reserve(estimated_verses / 10); // estimate unique words
    }

    // Load verses into local data structures
    if (j.contains("books") && j["books"].is_array()) {
        for (const auto& book_json : j["books"]) {
            std::string book_name = book_json.value("name", "Unknown Book");
            if (book_json.contains("chapters") && book_json["chapters"].is_array()) {
                for (const auto& chapter_json : book_json["chapters"]) {
                    int chapter_num = chapter_json.value("chapter", 1);
                    if (chapter_json.contains("verses") && chapter_json["verses"].is_array()) {
                        for (const auto& verse_json : chapter_json["verses"]) {
                            Verse v{
                                book_name,
                                chapter_num,
                                verse_json.value("verse", 1),
                                verse_json.value("text", "")
                            };
                            std::string key = makeKey(v.book, v.chapter, v.verse);
                            
                            // Build keyword index before moving the verse
                            std::vector<std::string> tokens = tokenize(v.text);
                            for (const auto& token : tokens) {
                                local_keyword_index[token].push_back(key);
                            }
                            
                            local_verses[key] = std::move(v);
                        }
                    }
                }
            }
        }
    }
    
    // Now lock and move all data to shared structures
    {
        std::lock_guard<std::mutex> lock(data_mutex);
        
        // Check if translation already exists
        for (const auto& trans_info : available_translations) {
            if (trans_info.name == trans_name) {
                std::cout << "Translation " << trans_name << " already loaded, skipping." << std::endl;
                return;
            }
        }
        
        available_translations.push_back({trans_name, trans_abbr});
        
        // Move local data to shared structures
        verses[trans_name] = std::move(local_verses);
        keyword_index[trans_name] = std::move(local_keyword_index);
        
        std::cout << "Loaded translation: " << trans_name << " (" 
                  << verses[trans_name].size() << " verses)" << std::endl;
    }
}

bool VerseFinder::saveTranslation(const std::string& json_data, const std::string& filename) {
    if (translations_dir.empty()) {
        std::cerr << "Translations directory not set" << std::endl;
        return false;
    }
    
    std::string full_path = translations_dir + "/" + filename;
    
    // Ensure the filename has .json extension
    if (full_path.substr(full_path.length() - 5) != ".json") {
        full_path += ".json";
    }
    
    std::ofstream file(full_path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not create file " << full_path << std::endl;
        return false;
    }
    
    file << json_data;
    file.close();
    
    std::cout << "Saved translation to: " << full_path << std::endl;
    return true;
}

std::string VerseFinder::getAdjacentVerse(const std::string& reference, const std::string& translation, int direction) const {
    if (!isReady()) return "";
    
    // Parse the reference to extract book, chapter, and verse
    size_t space_pos = reference.find_last_of(' ');
    if (space_pos == std::string::npos) return "";
    
    std::string book = reference.substr(0, space_pos);
    std::string chapter_verse = reference.substr(space_pos + 1);
    
    size_t colon_pos = chapter_verse.find(':');
    if (colon_pos == std::string::npos) return "";
    
    int chapter = std::stoi(chapter_verse.substr(0, colon_pos));
    int verse = std::stoi(chapter_verse.substr(colon_pos + 1));
    
    // Normalize book name
    std::string normalized_book = normalizeBookName(book);
    
    // Navigate step by step to handle multiple movements and chapter boundaries
    int current_chapter = chapter;
    int current_verse = verse;
    int steps_remaining = std::abs(direction);
    int step_direction = (direction > 0) ? 1 : -1;
    
    for (int i = 0; i < steps_remaining; i++) {
        int next_verse = current_verse + step_direction;
        
        if (step_direction > 0) {
            // Moving forward
            if (!verseExists(normalized_book, current_chapter, next_verse, translation)) {
                // Try next chapter, verse 1
                current_chapter++;
                next_verse = 1;
                if (!verseExists(normalized_book, current_chapter, next_verse, translation)) {
                    // Reached end of book or invalid book
                    if (i == 0) return ""; // Couldn't move at all
                    break; // Stop here with current position
                }
            }
        } else {
            // Moving backward
            if (next_verse < 1) {
                // Go to previous chapter's last verse
                current_chapter--;
                if (current_chapter < 1) {
                    // Reached beginning of book
                    if (i == 0) return ""; // Couldn't move at all
                    break; // Stop here with current position
                }
                next_verse = getLastVerseInChapter(normalized_book, current_chapter, translation);
                if (next_verse == 0) {
                    // Chapter doesn't exist
                    if (i == 0) return ""; // Couldn't move at all
                    break; // Stop here with current position
                }
            }
        }
        
        current_verse = next_verse;
    }
    
    // Construct new reference and search for it
    std::string new_reference = normalized_book + " " + std::to_string(current_chapter) + ":" + std::to_string(current_verse);
    std::string result = searchByReference(new_reference, translation);
    
    if (result != "Verse not found.") {
        return new_reference + ": " + result;
    }
    
    return "";
}

bool VerseFinder::verseExists(const std::string& book, int chapter, int verse, const std::string& translation) const {
    if (!isReady()) return false;
    
    std::string key = makeKey(book, chapter, verse);
    auto trans_it = verses.find(translation);
    if (trans_it != verses.end()) {
        return trans_it->second.find(key) != trans_it->second.end();
    }
    return false;
}

int VerseFinder::getLastVerseInChapter(const std::string& book, int chapter, const std::string& translation) const {
    if (!isReady()) return 0;
    
    auto trans_it = verses.find(translation);
    if (trans_it == verses.end()) return 0;
    
    int last_verse = 0;
    for (const auto& verse_pair : trans_it->second) {
        const Verse& v = verse_pair.second;
        if (v.book == book && v.chapter == chapter) {
            last_verse = std::max(last_verse, v.verse);
        }
    }
    
    return last_verse;
}

int VerseFinder::getLastChapterInBook(const std::string& book, const std::string& translation) const {
    if (!isReady()) return 0;
    
    auto trans_it = verses.find(translation);
    if (trans_it == verses.end()) return 0;
    
    int last_chapter = 0;
    for (const auto& verse_pair : trans_it->second) {
        const Verse& v = verse_pair.second;
        if (v.book == book) {
            last_chapter = std::max(last_chapter, v.chapter);
        }
    }
    
    return last_chapter;
}

void VerseFinder::clearSearchCache() {
    search_cache.clear();
}

void VerseFinder::setBenchmark(PerformanceBenchmark* bench) {
    benchmark = bench;
}

PerformanceBenchmark* VerseFinder::getBenchmark() const {
    return benchmark;
}

void VerseFinder::printPerformanceStats() const {
    if (benchmark) {
        benchmark->printSummary();
        
        // Print cache statistics
        std::cout << "\n=== Search Cache Statistics ===\n";
        std::cout << "Cache size: " << search_cache.size() << "/" << search_cache.maxSize() << "\n";
        std::cout << "Hit rate: " << std::fixed << std::setprecision(2) << (search_cache.hitRate() * 100) << "%\n";
        std::cout << std::endl;
    }
}

void VerseFinder::enableFuzzySearch(bool enable) {
    fuzzy_search_enabled = enable;
    std::cout << "Fuzzy search " << (enable ? "enabled" : "disabled") << std::endl;
}

bool VerseFinder::isFuzzySearchEnabled() const {
    return fuzzy_search_enabled;
}

void VerseFinder::setFuzzySearchOptions(const FuzzySearchOptions& options) {
    fuzzy_search.setOptions(options);
}

const FuzzySearchOptions& VerseFinder::getFuzzySearchOptions() const {
    return fuzzy_search.getOptions();
}

std::vector<std::string> VerseFinder::searchByKeywordsFuzzy(const std::string& query, const std::string& translation) const {
    if (!isReady()) return {"Bible is loading..."};
    if (!fuzzy_search_enabled) return searchByKeywords(query, translation);
    
    BENCHMARK_SCOPE("fuzzy_keyword_search");
    
    // First try exact search
    std::vector<std::string> exact_results = searchByKeywords(query, translation);
    
    // If we have good exact results, return them immediately
    if (!exact_results.empty() && exact_results[0] != "No matching verses found.") {
        return exact_results;
    }
    
    // Get translation data
    auto trans_it = verses.find(translation);
    if (trans_it == verses.end()) {
        return {"Translation not found."};
    }
    
    auto keyword_it = keyword_index.find(translation);
    if (keyword_it == keyword_index.end()) {
        return {"Translation index not found."};
    }
    
    const auto& translation_verses = trans_it->second;
    const auto& index = keyword_it->second;
    
    // Tokenize query for smart candidate selection
    auto query_tokens = SearchOptimizer::optimizedTokenize(query);
    std::unordered_set<std::string> candidate_verses;
    
    // Strategy 1: Find candidates through fuzzy word matching in the index
    for (const auto& token : query_tokens) {
        // Collect a sample of index words for fuzzy matching (performance limited)
        std::vector<std::string> sample_words;
        sample_words.reserve(200); // Limit vocabulary sample for performance
        
        for (const auto& entry : index) {
            sample_words.push_back(entry.first);
            if (sample_words.size() >= 200) break; // Hard limit for performance
        }
        
        // Set lower confidence for word matching to cast a wider net
        FuzzySearchOptions word_options = fuzzy_search.getOptions();
        word_options.minConfidence = 0.4; // Lower threshold for word matching
        word_options.maxSuggestions = 10; // More word suggestions
        FuzzySearch word_matcher(word_options);
        
        std::vector<FuzzyMatch> word_matches = word_matcher.findMatches(token, sample_words);
        
        // For each matched word, add its verses to candidates
        for (const auto& match : word_matches) {
            auto word_it = index.find(match.text);
            if (word_it != index.end()) {
                for (const auto& verse_key : word_it->second) {
                    candidate_verses.insert(verse_key);
                    // Limit total candidates for performance
                    if (candidate_verses.size() >= 300) break;
                }
            }
            if (candidate_verses.size() >= 300) break;
        }
        
        // Early termination if we have enough candidates
        if (candidate_verses.size() >= 300) break;
    }
    
    // Strategy 2: If still too few candidates, add a sample from full verses
    if (candidate_verses.size() < 50) {
        size_t verse_count = 0;
        for (const auto& verse_pair : translation_verses) {
            candidate_verses.insert(verse_pair.first);
            if (++verse_count >= 200) break; // Limit sample for performance
        }
    }
    
    // Now perform fuzzy text matching only on the candidate set
    std::vector<std::string> candidate_texts;
    std::vector<std::string> candidate_keys;
    candidate_texts.reserve(candidate_verses.size());
    candidate_keys.reserve(candidate_verses.size());
    
    for (const auto& verse_key : candidate_verses) {
        auto verse_it = translation_verses.find(verse_key);
        if (verse_it != translation_verses.end()) {
            candidate_texts.push_back(verse_it->second.text);
            candidate_keys.push_back(verse_key);
        }
    }
    
    // Perform fuzzy search on the much smaller candidate set
    std::vector<FuzzyMatch> fuzzy_matches = fuzzy_search.findMatches(query, candidate_texts);
    
    // Build results with confidence indicators - use direct indexing for performance
    std::vector<std::string> results;
    results.reserve(fuzzy_matches.size());
    
    for (size_t i = 0; i < fuzzy_matches.size(); ++i) {
        const auto& match = fuzzy_matches[i];
        
        // Find the corresponding verse key using direct indexing where possible
        for (size_t j = 0; j < candidate_texts.size(); ++j) {
            if (candidate_texts[j] == match.text) {
                std::string confidence_indicator;
                if (match.matchType == "fuzzy") {
                    confidence_indicator = " [~" + std::to_string(static_cast<int>(match.confidence * 100)) + "%]";
                } else if (match.matchType == "phonetic") {
                    confidence_indicator = " [♪]";
                } else if (match.matchType == "partial") {
                    confidence_indicator = " [...]";
                }
                results.push_back(candidate_keys[j] + confidence_indicator + ": " + match.text);
                break; // Found the match, no need to continue searching
            }
        }
    }
    
    return results.empty() ? std::vector<std::string>{"No fuzzy matches found."} : results;
}

std::vector<FuzzyMatch> VerseFinder::findBookNameSuggestions(const std::string& query) const {
    if (!isReady() || !fuzzy_search_enabled) return {};
    
    // Collect all unique book names from loaded translations
    std::set<std::string> book_names_set;
    for (const auto& translation_pair : verses) {
        for (const auto& verse_pair : translation_pair.second) {
            book_names_set.insert(verse_pair.second.book);
        }
    }
    
    // Also include book aliases
    for (const auto& alias_pair : book_aliases) {
        book_names_set.insert(alias_pair.first);
        book_names_set.insert(alias_pair.second);
    }
    
    std::vector<std::string> book_names(book_names_set.begin(), book_names_set.end());
    return fuzzy_search.findBookMatches(query, book_names);
}

std::vector<std::string> VerseFinder::generateQuerySuggestions(const std::string& query, const std::string& translation) const {
    if (!isReady() || !fuzzy_search_enabled) return {};
    
    // Generate suggestions based on keywords from the translation
    auto trans_it = keyword_index.find(translation);
    if (trans_it == keyword_index.end()) {
        return {};
    }
    
    std::vector<std::string> keywords;
    for (const auto& keyword_pair : trans_it->second) {
        keywords.push_back(keyword_pair.first);
    }
    
    return fuzzy_search.generateSuggestions(query, keywords);
}
