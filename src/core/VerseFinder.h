
#ifndef VERSEFINDER_H
#define VERSEFINDER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <future>
#include <atomic>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

struct Verse {
    std::string book;
    int chapter;
    int verse;
    std::string text;
};

struct TranslationInfo {
    std::string name;
    std::string abbreviation;
    // Add other metadata if needed, like source URL, licensing info
};

class VerseFinder {
private:
    std::unordered_map<std::string, std::unordered_map<std::string, Verse>> verses;
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::string>>> keyword_index;
    std::vector<TranslationInfo> available_translations;
    std::unordered_map<std::string, std::string> book_aliases;
    std::future<void> loading_future;
    std::atomic<bool> data_loaded{false};
    std::string translations_dir;

    void loadBibleInternal(const std::string& filename);
    void loadTranslationsFromDirectory(const std::string& dir_path);
    void loadSingleTranslation(const std::string& filename);
    std::string normalizeBookName(const std::string& book) const;
    std::string normalizeReference(const std::string& reference) const;
    std::string makeKey(const std::string& book, int chapter, int verse) const;
    static std::vector<std::string> tokenize(const std::string& text);

public:
    VerseFinder();
    void startLoading(const std::string& filename);
    void setTranslationsDirectory(const std::string& dir_path);
    void loadAllTranslations();
    bool isReady() const;
    std::string searchByReference(const std::string& reference, const std::string& translation) const;
    std::vector<std::string> searchByKeywords(const std::string& query, const std::string& translation) const;
    const std::vector<TranslationInfo>& getTranslations() const;
    void addTranslation(const std::string& json_data);
    bool saveTranslation(const std::string& json_data, const std::string& filename);
    
    // Navigation helper methods
    std::string getAdjacentVerse(const std::string& reference, const std::string& translation, int direction) const;
    bool verseExists(const std::string& book, int chapter, int verse, const std::string& translation) const;
    int getLastVerseInChapter(const std::string& book, int chapter, const std::string& translation) const;
    int getLastChapterInBook(const std::string& book, const std::string& translation) const;
};

#endif //VERSEFINDER_H
