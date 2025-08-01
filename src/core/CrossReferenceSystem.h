#ifndef CROSSREFERENCESYSTEM_H
#define CROSSREFERENCESYSTEM_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

struct CrossReference {
    std::string sourceVerse;
    std::string targetVerse;
    std::string relationship;  // "parallel", "quotation", "allusion", "theme"
    double confidence;
    std::vector<std::string> commonKeywords;
};

struct ParallelPassage {
    std::vector<std::string> verses;
    std::string theme;
    double coherenceScore;
};

class CrossReferenceSystem {
private:
    // Cross-reference mappings
    std::unordered_map<std::string, std::vector<CrossReference>> crossReferences;
    std::unordered_map<std::string, std::vector<std::string>> thematicGroups;
    std::unordered_map<std::string, std::vector<std::string>> parallelPassages;
    
    // Common biblical themes and their keywords
    std::unordered_map<std::string, std::vector<std::string>> biblicalThemes;
    
    // Initialize with common biblical cross-references and parallel passages
    void initializeCommonCrossReferences();
    void initializeBiblicalThemes();
    void initializeParallelPassages();
    
    // Analysis helpers
    double calculateSimilarity(const std::string& text1, const std::string& text2) const;
    std::vector<std::string> extractThemes(const std::string& verseText) const;
    std::vector<std::string> findCommonKeywords(const std::string& text1, const std::string& text2) const;
    
public:
    CrossReferenceSystem();
    
    // Cross-reference discovery
    std::vector<CrossReference> findCrossReferences(const std::string& verseKey, 
                                                   const std::unordered_map<std::string, std::string>& allVerses) const;
    std::vector<std::string> findParallelPassages(const std::string& verseKey,
                                                 const std::unordered_map<std::string, std::string>& allVerses) const;
    std::vector<std::string> findThematicMatches(const std::string& verseKey,
                                               const std::unordered_map<std::string, std::string>& allVerses) const;
    
    // Relationship analysis
    std::string determineRelationshipType(const std::string& verse1, const std::string& verse2) const;
    std::vector<ParallelPassage> findParallelPassageGroups(const std::vector<std::string>& verseKeys,
                                                          const std::unordered_map<std::string, std::string>& allVerses) const;
    
    // Context expansion
    std::vector<std::string> expandContext(const std::string& verseKey, int beforeCount = 2, int afterCount = 2) const;
    std::vector<std::string> getSurroundingVerses(const std::string& verseKey) const;
    
    // Theme-based discovery
    std::vector<std::string> findVersesByTheme(const std::string& theme,
                                             const std::unordered_map<std::string, std::string>& allVerses) const;
    std::vector<std::string> getRelatedThemes(const std::string& theme) const;
    
    // Custom cross-reference management
    void addCrossReference(const std::string& source, const std::string& target, 
                          const std::string& relationship, double confidence = 0.8);
    void removeCrossReference(const std::string& source, const std::string& target);
    void addThematicGroup(const std::string& theme, const std::vector<std::string>& verses);
    
    // Import/Export functionality
    void loadCrossReferenceData(const std::string& jsonData);
    std::string exportCrossReferenceData() const;
    
    // Statistics and analysis
    int getCrossReferenceCount() const;
    std::vector<std::pair<std::string, int>> getMostReferencedVerses() const;
    std::vector<std::string> getMostCommonThemes() const;
};

#endif // CROSSREFERENCESYSTEM_H