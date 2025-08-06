#include "interfaces/PluginInterfaces.h"
#include "api/PluginAPI.h"
#include <algorithm>
#include <regex>

using namespace PluginSystem;

class EnhancedSearchPlugin : public ISearchPlugin {
private:
    PluginInfo info;
    PluginState state;
    PluginAPI* api;
    std::string last_error;
    
public:
    EnhancedSearchPlugin() : state(PluginState::UNLOADED), api(nullptr) {
        info.name = "Enhanced Search Plugin";
        info.description = "Provides advanced search capabilities including regex, wildcards, and semantic search";
        info.author = "VerseFinder Team";
        info.version = {1, 0, 0};
        info.website = "https://versefinder.com/plugins/enhanced-search";
        info.tags = {"search", "regex", "semantic", "wildcards"};
    }
    
    // IPlugin interface
    bool initialize() override {
        state = PluginState::LOADED;
        return true;
    }
    
    void shutdown() override {
        state = PluginState::UNLOADED;
        api = nullptr;
    }
    
    const PluginInfo& getInfo() const override {
        return info;
    }
    
    bool configure(const PluginConfig& config) override {
        // Configuration could include regex flags, search limits, etc.
        return true;
    }
    
    void onActivate() override {
        state = PluginState::ACTIVE;
    }
    
    void onDeactivate() override {
        state = PluginState::LOADED;
    }
    
    PluginState getState() const override {
        return state;
    }
    
    std::string getLastError() const override {
        return last_error;
    }
    
    // ISearchPlugin interface
    std::vector<std::string> search(const std::string& query, const std::string& translation) override {
        if (!api) {
            last_error = "Plugin API not available";
            return {};
        }
        
        // Determine search type based on query patterns
        if (query.find("regex:") == 0) {
            return regexSearch(query.substr(6), translation);
        } else if (query.find("wildcard:") == 0) {
            return wildcardSearch(query.substr(9), translation);
        } else if (query.find("semantic:") == 0) {
            return api->searchSemantic(query.substr(9), translation);
        } else if (query.find("fuzzy:") == 0) {
            return fuzzySearch(query.substr(6), translation);
        } else {
            // Enhanced keyword search with synonyms
            return enhancedKeywordSearch(query, translation);
        }
    }
    
    std::vector<std::string> searchAdvanced(const std::string& query, 
                                           const std::string& translation,
                                           const std::unordered_map<std::string, std::string>& options) override {
        if (!api) {
            last_error = "Plugin API not available";
            return {};
        }
        
        std::string searchType = getOption(options, "type", "keyword");
        bool caseSensitive = getOption(options, "case_sensitive", "false") == "true";
        bool wholeWords = getOption(options, "whole_words", "false") == "true";
        int maxResults = std::stoi(getOption(options, "max_results", "100"));
        
        std::vector<std::string> results;
        
        if (searchType == "regex") {
            results = regexSearch(query, translation, caseSensitive);
        } else if (searchType == "wildcard") {
            results = wildcardSearch(query, translation, caseSensitive);
        } else if (searchType == "semantic") {
            results = api->searchSemantic(query, translation);
        } else if (searchType == "phrase") {
            results = phraseSearch(query, translation, caseSensitive, wholeWords);
        } else {
            results = enhancedKeywordSearch(query, translation);
        }
        
        // Limit results
        if (results.size() > static_cast<size_t>(maxResults)) {
            results.resize(maxResults);
        }
        
        return results;
    }
    
    bool supportsTranslation(const std::string& translation) const override {
        // Support all translations
        return true;
    }
    
    std::vector<std::string> getSupportedOptions() const override {
        return {
            "type", "case_sensitive", "whole_words", "max_results", 
            "include_context", "sort_by_relevance"
        };
    }
    
    std::string getSearchDescription() const override {
        return "Enhanced search with regex, wildcards, semantic search, and advanced filtering options";
    }
    
    double getSearchQuality(const std::string& query) const override {
        // Rate search quality based on query complexity
        if (query.find("regex:") == 0 || query.find("semantic:") == 0) {
            return 0.9; // High quality for advanced searches
        } else if (query.find("wildcard:") == 0) {
            return 0.7; // Good quality for wildcard searches
        } else if (query.length() > 10) {
            return 0.8; // Good quality for longer queries
        } else {
            return 0.5; // Medium quality for simple searches
        }
    }
    
    void setAPI(PluginAPI* pluginAPI) {
        api = pluginAPI;
    }
    
private:
    std::vector<std::string> regexSearch(const std::string& pattern, const std::string& translation, bool caseSensitive = false) {
        try {
            std::regex::flag_type flags = std::regex::optimize;
            if (!caseSensitive) {
                flags |= std::regex::icase;
            }
            
            std::regex regex(pattern, flags);
            std::vector<std::string> results;
            
            // Get all verses and search through them
            // This is a simplified implementation - in reality, we'd need to iterate through all verses
            auto allVerses = api->searchByKeywords("", translation); // Get all verses (simplified)
            
            for (const auto& verse : allVerses) {
                if (std::regex_search(verse, regex)) {
                    results.push_back(verse);
                }
            }
            
            return results;
        } catch (const std::regex_error& e) {
            last_error = "Invalid regex pattern: " + std::string(e.what());
            return {};
        }
    }
    
    std::vector<std::string> wildcardSearch(const std::string& pattern, const std::string& translation, bool caseSensitive = false) {
        // Convert wildcard pattern to regex
        std::string regexPattern = pattern;
        
        // Escape special regex characters except * and ?
        std::string specialChars = "^$+{}[]|()\\";
        for (char c : specialChars) {
            size_t pos = 0;
            std::string charStr(1, c);
            std::string escaped = "\\" + charStr;
            while ((pos = regexPattern.find(charStr, pos)) != std::string::npos) {
                regexPattern.replace(pos, 1, escaped);
                pos += escaped.length();
            }
        }
        
        // Convert wildcards to regex
        size_t pos = 0;
        while ((pos = regexPattern.find('*', pos)) != std::string::npos) {
            regexPattern.replace(pos, 1, ".*");
            pos += 2;
        }
        pos = 0;
        while ((pos = regexPattern.find('?', pos)) != std::string::npos) {
            regexPattern.replace(pos, 1, ".");
            pos += 1;
        }
        
        return regexSearch(regexPattern, translation, caseSensitive);
    }
    
    std::vector<std::string> fuzzySearch(const std::string& query, const std::string& translation) {
        // Implement fuzzy search using edit distance
        // This is a simplified implementation
        auto exactResults = api->searchByKeywords(query, translation);
        
        if (!exactResults.empty()) {
            return exactResults;
        }
        
        // Try variations with common misspellings/typos
        std::vector<std::string> variations = generateQueryVariations(query);
        
        for (const auto& variation : variations) {
            auto results = api->searchByKeywords(variation, translation);
            if (!results.empty()) {
                return results;
            }
        }
        
        return {};
    }
    
    std::vector<std::string> enhancedKeywordSearch(const std::string& query, const std::string& translation) {
        auto results = api->searchByKeywords(query, translation);
        
        // Add synonym search
        auto synonymResults = searchWithSynonyms(query, translation);
        
        // Merge results and remove duplicates
        results.insert(results.end(), synonymResults.begin(), synonymResults.end());
        std::sort(results.begin(), results.end());
        results.erase(std::unique(results.begin(), results.end()), results.end());
        
        return results;
    }
    
    std::vector<std::string> phraseSearch(const std::string& phrase, const std::string& translation, 
                                         bool caseSensitive, bool wholeWords) {
        // Search for exact phrase matches
        std::string searchPattern = phrase;
        
        if (wholeWords) {
            searchPattern = "\\b" + phrase + "\\b";
        }
        
        return regexSearch(searchPattern, translation, caseSensitive);
    }
    
    std::vector<std::string> searchWithSynonyms(const std::string& query, const std::string& translation) {
        // Simple synonym mapping - in a real implementation, this would use a proper thesaurus
        std::unordered_map<std::string, std::vector<std::string>> synonyms = {
            {"love", {"charity", "affection", "devotion"}},
            {"peace", {"calm", "tranquility", "serenity"}},
            {"joy", {"happiness", "gladness", "rejoice"}},
            {"hope", {"faith", "trust", "confidence"}},
            {"fear", {"afraid", "terror", "dread"}},
            {"death", {"die", "perish", "deceased"}},
            {"life", {"living", "alive", "existence"}},
            {"light", {"bright", "illuminate", "shine"}},
            {"dark", {"darkness", "shadow", "night"}}
        };
        
        std::vector<std::string> results;
        auto it = synonyms.find(query);
        
        if (it != synonyms.end()) {
            for (const auto& synonym : it->second) {
                auto synonymResults = api->searchByKeywords(synonym, translation);
                results.insert(results.end(), synonymResults.begin(), synonymResults.end());
            }
        }
        
        return results;
    }
    
    std::vector<std::string> generateQueryVariations(const std::string& query) {
        std::vector<std::string> variations;
        
        // Add common variations
        variations.push_back(query.substr(0, query.length() - 1)); // Remove last character
        if (query.length() > 1) {
            std::string swapped = query;
            std::swap(swapped[query.length() - 1], swapped[query.length() - 2]); // Swap last two chars
            variations.push_back(swapped);
        }
        
        // Add case variations
        std::string lowercase = query;
        std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(), ::tolower);
        variations.push_back(lowercase);
        
        std::string uppercase = query;
        std::transform(uppercase.begin(), uppercase.end(), uppercase.begin(), ::toupper);
        variations.push_back(uppercase);
        
        return variations;
    }
    
    std::string getOption(const std::unordered_map<std::string, std::string>& options, 
                         const std::string& key, const std::string& defaultValue) const {
        auto it = options.find(key);
        return it != options.end() ? it->second : defaultValue;
    }
};

// Plugin factory functions
extern "C" {
    PluginSystem::IPlugin* createPlugin() {
        return new EnhancedSearchPlugin();
    }
    
    void destroyPlugin(PluginSystem::IPlugin* plugin) {
        delete plugin;
    }
    
    const char* getPluginApiVersion() {
        return "1.0";
    }
    
    const char* getPluginType() {
        return "search";
    }
}