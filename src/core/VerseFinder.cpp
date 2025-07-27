
#include "VerseFinder.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <thread>
#include <filesystem>

VerseFinder::VerseFinder() {
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
                for (const auto& token : tokenize(v.text)) {
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
    
    // If no alias found, return the original book name
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
    auto tokens = tokenize(query);
    if (tokens.empty()) return {"No keywords provided."};

    auto trans_it = keyword_index.find(translation);
    if (trans_it == keyword_index.end()) {
        return {"Translation not found."};
    }
    const auto& index = trans_it->second;

    auto token_it = index.find(tokens[0]);
    if (token_it == index.end()) {
        return {"No matching verses found."};
    }

    std::vector<std::string> common_refs = token_it->second;
    std::sort(common_refs.begin(), common_refs.end());

    for (size_t i = 1; i < tokens.size(); ++i) {
        token_it = index.find(tokens[i]);
        if (token_it == index.end()) {
            return {"No matching verses found."};
        }

        std::vector<std::string> refs = token_it->second;
        std::sort(refs.begin(), refs.end());

        std::vector<std::string> intersection;
        std::set_intersection(common_refs.begin(), common_refs.end(),
                              refs.begin(), refs.end(),
                              std::back_inserter(intersection));
        common_refs = std::move(intersection);

        if (common_refs.empty()) {
            break;
        }
    }

    std::vector<std::string> results;
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    for (const auto& ref : common_refs) {
        const auto& verse_text = verses.at(translation).at(ref).text;
        std::string lower_text = verse_text;
        std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(),
                       [](unsigned char c){ return std::tolower(c); });

        if (lower_text.find(lower_query) != std::string::npos) {
            results.push_back(ref + ": " + verse_text);
        }
    }
    return results.empty() ? std::vector<std::string>{"No matching verses found."} : results;
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
                for (const auto& token : tokenize(v.text)) {
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
    
    // Clear existing translations
    available_translations.clear();
    verses.clear();
    keyword_index.clear();
    
    // Load all JSON files in the directory
    for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::cout << "Loading translation: " << entry.path().filename() << std::endl;
            loadSingleTranslation(entry.path().string());
        }
    }
    
    // If no translations found in directory, try to load bible.json from parent directory
    if (available_translations.empty()) {
        std::string bible_file = dir_path + "/../bible.json";
        if (std::filesystem::exists(bible_file)) {
            std::cout << "Loading default bible.json" << std::endl;
            loadSingleTranslation(bible_file);
        }
    }
    
    data_loaded = true;
    std::cout << "Loaded " << available_translations.size() << " translations." << std::endl;
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
