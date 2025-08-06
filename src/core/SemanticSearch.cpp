#include "SemanticSearch.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cmath>

SemanticSearch::SemanticSearch() {
    initializeTopicKeywords();
    initializeQuestionPatterns();
    initializeContextualSituations();
    initializeBooleanPatterns();
    initializeStopWords();
    initializeSynonyms();
}

void SemanticSearch::initializeTopicKeywords() {
    topicKeywords = {
        {"love", {"love", "beloved", "charity", "affection", "devotion", "compassion", "caring", "tender", "kindness"}},
        {"hope", {"hope", "trust", "faith", "expectation", "confidence", "assurance", "promise", "future", "wait"}},
        {"peace", {"peace", "rest", "calm", "quiet", "still", "tranquil", "harmony", "reconciliation", "shalom"}},
        {"joy", {"joy", "rejoice", "glad", "happy", "delight", "celebration", "cheerful", "merry", "blessing"}},
        {"faith", {"faith", "believe", "trust", "confidence", "conviction", "assurance", "reliance", "hope"}},
        {"forgiveness", {"forgive", "pardon", "mercy", "grace", "redemption", "cleanse", "wash", "remission"}},
        {"salvation", {"salvation", "save", "redeem", "deliver", "rescue", "eternal", "life", "born", "again"}},
        {"strength", {"strength", "strong", "power", "mighty", "courage", "brave", "bold", "fortify", "endure"}},
        {"wisdom", {"wisdom", "wise", "understanding", "knowledge", "discernment", "prudence", "insight", "counsel"}},
        {"prayer", {"pray", "prayer", "petition", "supplication", "intercession", "request", "ask", "seek"}},
        {"comfort", {"comfort", "console", "encouragement", "solace", "relief", "ease", "support", "help"}},
        {"guidance", {"guide", "lead", "direct", "path", "way", "direction", "counsel", "instruction", "teach"}},
        {"protection", {"protect", "shield", "refuge", "fortress", "stronghold", "shelter", "defend", "guard"}},
        {"healing", {"heal", "healing", "restore", "recovery", "cure", "wholeness", "health", "mend"}},
        {"purpose", {"purpose", "calling", "mission", "destiny", "plan", "will", "work", "service", "ministry"}},
        {"suffering", {"suffer", "affliction", "tribulation", "trial", "persecution", "pain", "hardship", "burden"}},
        {"temptation", {"temptation", "tempt", "test", "trial", "overcome", "resist", "flee", "deliver"}},
        {"marriage", {"marriage", "husband", "wife", "spouse", "wedding", "bride", "bridegroom", "love"}},
        {"family", {"family", "children", "parents", "father", "mother", "son", "daughter", "household"}},
        {"money", {"money", "wealth", "riches", "treasure", "mammon", "gold", "silver", "giving", "tithe"}},
        {"work", {"work", "labor", "employment", "job", "service", "ministry", "calling", "vocation"}},
        {"death", {"death", "die", "grave", "tomb", "resurrection", "eternal", "life", "heaven", "paradise"}},
        {"fear", {"fear", "afraid", "terror", "dread", "anxiety", "worry", "concern", "trouble"}},
        {"anger", {"anger", "wrath", "fury", "rage", "indignation", "displeasure", "upset", "mad"}},
        {"patience", {"patience", "patient", "endure", "persevere", "wait", "long-suffering", "steadfast"}},
        {"humility", {"humble", "humility", "meek", "lowly", "modest", "submissive", "gentle", "poor"}},
        {"justice", {"justice", "just", "righteous", "fair", "judgment", "vindication", "equity", "right"}},
        {"mercy", {"mercy", "merciful", "compassion", "pity", "kindness", "grace", "loving-kindness"}},
        {"truth", {"truth", "true", "honest", "sincere", "genuine", "real", "faithful", "trustworthy"}},
        {"obedience", {"obey", "obedience", "submit", "follow", "keep", "commandments", "law", "will"}}
    };
}

void SemanticSearch::initializeQuestionPatterns() {
    questionPatterns = {
        {"what does.*say about", "topical"},
        {"how to.*", "guidance"},
        {"what should.*do", "guidance"},
        {"where.*find", "topical"},
        {"who.*", "character"},
        {"why.*", "understanding"},
        {"when.*", "prophecy"},
        {"verses about.*", "topical"},
        {"verses for.*", "contextual"},
        {"bible.*comfort", "comfort"},
        {"bible.*hope", "hope"},
        {"bible.*strength", "strength"},
        {"scripture.*", "topical"}
    };
}

void SemanticSearch::initializeContextualSituations() {
    contextualSituations = {
        {"difficult times", {"comfort", "strength", "hope", "perseverance", "faith", "trust"}},
        {"illness", {"healing", "comfort", "faith", "peace", "strength", "prayer"}},
        {"grief", {"comfort", "hope", "peace", "eternal life", "resurrection", "love"}},
        {"anxiety", {"peace", "trust", "fear not", "comfort", "strength", "prayer"}},
        {"depression", {"hope", "joy", "comfort", "love", "purpose", "strength"}},
        {"financial problems", {"provision", "trust", "faith", "money", "giving", "contentment"}},
        {"relationship issues", {"love", "forgiveness", "patience", "marriage", "family", "reconciliation"}},
        {"loneliness", {"comfort", "love", "presence", "friendship", "community", "peace"}},
        {"decision making", {"wisdom", "guidance", "prayer", "discernment", "will", "peace"}},
        {"temptation", {"strength", "resistance", "prayer", "purity", "holiness", "overcome"}},
        {"doubt", {"faith", "trust", "evidence", "assurance", "hope", "belief"}},
        {"anger", {"forgiveness", "patience", "peace", "self-control", "love", "mercy"}},
        {"fear", {"courage", "strength", "protection", "peace", "trust", "faith"}},
        {"guilt", {"forgiveness", "grace", "mercy", "cleansing", "redemption", "peace"}},
        {"workplace", {"integrity", "work", "service", "witness", "patience", "wisdom"}},
        {"parenting", {"wisdom", "patience", "love", "discipline", "family", "children"}},
        {"marriage", {"love", "patience", "forgiveness", "unity", "respect", "commitment"}},
        {"loss", {"comfort", "hope", "eternal life", "peace", "strength", "presence"}}
    };
}

void SemanticSearch::initializeBooleanPatterns() {
    booleanPatterns = {
        std::regex(R"(\b(AND|and|\&\&|\+)\b)"),
        std::regex(R"(\b(OR|or|\|\|)\b)"),
        std::regex(R"(\b(NOT|not|\!|-)\b)")
    };
}

void SemanticSearch::initializeStopWords() {
    stopWords = {
        "a", "an", "and", "are", "as", "at", "be", "by", "for", "from", "has", "he", "in", "is", "it",
        "its", "of", "on", "that", "the", "to", "was", "will", "with", "the", "this", "these", "those",
        "what", "where", "when", "why", "how", "who", "which", "does", "do", "did", "can", "could",
        "should", "would", "may", "might", "must", "shall", "about", "up", "out", "if", "no", "all"
    };
}

void SemanticSearch::initializeSynonyms() {
    synonyms = {
        {"happy", {"joy", "glad", "cheerful", "delight", "blessed"}},
        {"sad", {"sorrow", "grief", "mourn", "weep", "lament"}},
        {"scared", {"fear", "afraid", "terror", "dread", "anxious"}},
        {"strong", {"strength", "power", "mighty", "courage", "bold"}},
        {"help", {"aid", "assist", "support", "comfort", "deliver"}},
        {"good", {"righteous", "holy", "pure", "just", "perfect"}},
        {"bad", {"evil", "wicked", "sin", "iniquity", "wrong"}},
        {"money", {"wealth", "riches", "treasure", "mammon", "gold"}},
        {"sick", {"illness", "disease", "infirmity", "weakness", "affliction"}},
        {"tired", {"weary", "exhausted", "burden", "rest", "sleep"}},
        {"angry", {"wrath", "fury", "rage", "indignation", "mad"}},
        {"lost", {"wandering", "astray", "confused", "seeking", "found"}},
        {"alone", {"lonely", "solitary", "isolated", "forsaken", "abandoned"}},
        {"difficult", {"hard", "trouble", "trial", "tribulation", "challenging"}}
    };
}

std::vector<std::string> SemanticSearch::tokenizeAndFilter(const std::string& query) const {
    std::vector<std::string> tokens;
    std::istringstream iss(query);
    std::string token;
    
    while (iss >> token) {
        // Convert to lowercase
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
        
        // Remove punctuation except for biblical references
        std::string cleaned;
        for (char c : token) {
            if (std::isalnum(c) || c == ':') {
                cleaned += c;
            }
        }
        
        if (!cleaned.empty() && stopWords.find(cleaned) == stopWords.end()) {
            tokens.push_back(cleaned);
        }
    }
    
    return tokens;
}

std::string SemanticSearch::normalizeQuery(const std::string& query) const {
    std::string normalized = query;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // Remove extra whitespace
    std::regex ws_re("\\s+");
    normalized = std::regex_replace(normalized, ws_re, " ");
    
    // Trim leading/trailing whitespace
    normalized.erase(0, normalized.find_first_not_of(" \t\n\r"));
    normalized.erase(normalized.find_last_not_of(" \t\n\r") + 1);
    
    return normalized;
}

QueryIntent::Type SemanticSearch::detectQueryType(const std::string& query) const {
    std::string normalized = normalizeQuery(query);
    
    // Check for biblical reference pattern
    std::regex ref_pattern(R"(\b\w+\s+\d+:\d+\b)");
    if (std::regex_search(normalized, ref_pattern)) {
        return QueryIntent::REFERENCE_LOOKUP;
    }
    
    // Check for boolean operators
    for (const auto& pattern : booleanPatterns) {
        if (std::regex_search(normalized, pattern)) {
            return QueryIntent::BOOLEAN_SEARCH;
        }
    }
    
    // Check for question patterns
    for (const auto& pattern : questionPatterns) {
        std::regex question_regex(pattern.first, std::regex_constants::icase);
        if (std::regex_search(normalized, question_regex)) {
            return QueryIntent::QUESTION_BASED;
        }
    }
    
    // Check for topical search patterns
    if (normalized.find("about") != std::string::npos || 
        normalized.find("verses about") != std::string::npos ||
        normalized.find("scripture about") != std::string::npos) {
        return QueryIntent::TOPICAL_SEARCH;
    }
    
    // Check for contextual patterns
    if (normalized.find("for") != std::string::npos || 
        normalized.find("during") != std::string::npos ||
        normalized.find("when") != std::string::npos) {
        return QueryIntent::CONTEXTUAL_REQUEST;
    }
    
    // Check if query contains multiple topic keywords (semantic search)
    std::vector<std::string> tokens = tokenizeAndFilter(normalized);
    int topicMatches = 0;
    for (const auto& token : tokens) {
        for (const auto& topicPair : topicKeywords) {
            if (std::find(topicPair.second.begin(), topicPair.second.end(), token) != topicPair.second.end()) {
                topicMatches++;
                break;
            }
        }
    }
    
    if (topicMatches >= 2) {
        return QueryIntent::SEMANTIC_SEARCH;
    } else if (topicMatches >= 1) {
        return QueryIntent::TOPICAL_SEARCH;
    }
    
    // Default to keyword search
    return QueryIntent::KEYWORD_SEARCH;
}

std::vector<std::string> SemanticSearch::extractTopicsFromQuery(const std::string& query) const {
    std::vector<std::string> topics;
    std::vector<std::string> tokens = tokenizeAndFilter(query);
    
    for (const auto& token : tokens) {
        for (const auto& topicPair : topicKeywords) {
            if (std::find(topicPair.second.begin(), topicPair.second.end(), token) != topicPair.second.end()) {
                if (std::find(topics.begin(), topics.end(), topicPair.first) == topics.end()) {
                    topics.push_back(topicPair.first);
                }
            }
        }
    }
    
    return topics;
}

std::string SemanticSearch::extractSubjectFromQuestion(const std::string& query) const {
    std::string normalized = normalizeQuery(query);
    
    // Look for "about X" patterns
    std::regex about_pattern(R"(about\s+(\w+(?:\s+\w+)?))");
    std::smatch match;
    if (std::regex_search(normalized, match, about_pattern)) {
        return match[1].str();
    }
    
    // Look for other question patterns
    std::regex question_pattern(R"((?:what|how|where|when|why).*?(\w+(?:\s+\w+)?))");
    if (std::regex_search(normalized, match, question_pattern)) {
        return match[1].str();
    }
    
    return "";
}

std::vector<std::string> SemanticSearch::expandWithSynonyms(const std::vector<std::string>& keywords) const {
    std::vector<std::string> expanded = keywords;
    
    for (const auto& keyword : keywords) {
        auto it = synonyms.find(keyword);
        if (it != synonyms.end()) {
            for (const auto& synonym : it->second) {
                if (std::find(expanded.begin(), expanded.end(), synonym) == expanded.end()) {
                    expanded.push_back(synonym);
                }
            }
        }
    }
    
    return expanded;
}

QueryIntent SemanticSearch::parseQuery(const std::string& query) const {
    QueryIntent intent;
    intent.originalQuery = query;
    intent.type = detectQueryType(query);
    intent.keywords = tokenizeAndFilter(query);
    intent.topics = extractTopicsFromQuery(query);
    intent.subject = extractSubjectFromQuestion(query);
    intent.confidence = 0.8; // Base confidence
    
    // Adjust confidence based on pattern matches
    if (intent.type == QueryIntent::REFERENCE_LOOKUP) {
        intent.confidence = 0.95;
    } else if (intent.type == QueryIntent::QUESTION_BASED && !intent.subject.empty()) {
        intent.confidence = 0.9;
    } else if (intent.topics.empty() && intent.type != QueryIntent::KEYWORD_SEARCH) {
        intent.confidence = 0.6;
    }
    
    return intent;
}

std::vector<std::string> SemanticSearch::generateSemanticKeywords(const QueryIntent& intent) const {
    std::vector<std::string> semanticKeywords = intent.keywords;
    
    // Add topic-related keywords
    for (const auto& topic : intent.topics) {
        auto it = topicKeywords.find(topic);
        if (it != topicKeywords.end()) {
            for (const auto& keyword : it->second) {
                if (std::find(semanticKeywords.begin(), semanticKeywords.end(), keyword) == semanticKeywords.end()) {
                    semanticKeywords.push_back(keyword);
                }
            }
        }
    }
    
    // Expand with synonyms
    semanticKeywords = expandWithSynonyms(semanticKeywords);
    
    // Add contextual keywords if applicable
    if (intent.type == QueryIntent::CONTEXTUAL_REQUEST) {
        for (const auto& contextPair : contextualSituations) {
            for (const auto& keyword : intent.keywords) {
                if (contextPair.first.find(keyword) != std::string::npos) {
                    for (const auto& contextKeyword : contextPair.second) {
                        if (std::find(semanticKeywords.begin(), semanticKeywords.end(), contextKeyword) == semanticKeywords.end()) {
                            semanticKeywords.push_back(contextKeyword);
                        }
                    }
                }
            }
        }
    }
    
    return semanticKeywords;
}

std::vector<std::string> SemanticSearch::getRelatedTopics(const std::string& topic) const {
    std::vector<std::string> related;
    
    // Find topics with overlapping keywords
    auto topicIt = topicKeywords.find(topic);
    if (topicIt == topicKeywords.end()) return related;
    
    const auto& targetKeywords = topicIt->second;
    
    for (const auto& otherTopicPair : topicKeywords) {
        if (otherTopicPair.first == topic) continue;
        
        // Count overlapping keywords
        int overlap = 0;
        for (const auto& keyword : targetKeywords) {
            if (std::find(otherTopicPair.second.begin(), otherTopicPair.second.end(), keyword) != otherTopicPair.second.end()) {
                overlap++;
            }
        }
        
        // Include topics with significant overlap
        if (overlap >= 2) {
            related.push_back(otherTopicPair.first);
        }
    }
    
    return related;
}

SemanticSearch::BooleanQuery SemanticSearch::parseBooleanQuery(const std::string& query) const {
    BooleanQuery boolQuery;
    
    std::string workingQuery = query;
    std::transform(workingQuery.begin(), workingQuery.end(), workingQuery.begin(), ::tolower);
    
    // Split by NOT operators first
    std::regex not_pattern(R"(\s+(not|!|-)\s+)");
    std::vector<std::string> not_split;
    std::sregex_token_iterator iter(workingQuery.begin(), workingQuery.end(), not_pattern, -1);
    std::sregex_token_iterator end;
    
    for (; iter != end; ++iter) {
        std::string part = *iter;
        if (!part.empty()) {
            not_split.push_back(part);
        }
    }
    
    // Process each part for AND/OR
    for (size_t i = 0; i < not_split.size(); ++i) {
        std::string part = not_split[i];
        
        if (i == 0) {
            // First part contains AND/OR terms
            std::regex and_pattern(R"(\s+(and|&|&&|\+)\s+)");
            std::regex or_pattern(R"(\s+(or|\||\|\|)\s+)");
            
            // Split by OR first
            std::sregex_token_iterator or_iter(part.begin(), part.end(), or_pattern, -1);
            for (; or_iter != end; ++or_iter) {
                std::string or_part = *or_iter;
                if (!or_part.empty()) {
                    // Check if this part contains AND
                    std::sregex_token_iterator and_iter(or_part.begin(), or_part.end(), and_pattern, -1);
                    bool hasAnd = false;
                    for (; and_iter != end; ++and_iter) {
                        std::string and_part = *and_iter;
                        if (!and_part.empty()) {
                            std::vector<std::string> tokens = tokenizeAndFilter(and_part);
                            for (const auto& token : tokens) {
                                boolQuery.andTerms.push_back(token);
                            }
                            hasAnd = true;
                        }
                    }
                    
                    if (!hasAnd) {
                        std::vector<std::string> tokens = tokenizeAndFilter(or_part);
                        for (const auto& token : tokens) {
                            boolQuery.orTerms.push_back(token);
                        }
                    }
                }
            }
        } else {
            // Subsequent parts are NOT terms
            std::vector<std::string> tokens = tokenizeAndFilter(part);
            for (const auto& token : tokens) {
                boolQuery.notTerms.push_back(token);
            }
        }
    }
    
    return boolQuery;
}

std::vector<std::string> SemanticSearch::generateTopicalSuggestions(const std::string& input) const {
    std::vector<std::string> suggestions;
    std::string normalized = normalizeQuery(input);
    
    // Find topics that match the input
    for (const auto& topicPair : topicKeywords) {
        if (topicPair.first.find(normalized) != std::string::npos) {
            suggestions.push_back("verses about " + topicPair.first);
        }
    }
    
    // Find contextual situations
    for (const auto& contextPair : contextualSituations) {
        if (contextPair.first.find(normalized) != std::string::npos) {
            suggestions.push_back("verses for " + contextPair.first);
        }
    }
    
    return suggestions;
}

std::vector<std::string> SemanticSearch::generateContextualSuggestions(const std::string& situation) const {
    std::vector<std::string> suggestions;
    
    auto it = contextualSituations.find(situation);
    if (it != contextualSituations.end()) {
        for (const auto& topic : it->second) {
            suggestions.push_back("verses about " + topic);
        }
    }
    
    return suggestions;
}

void SemanticSearch::loadSemanticConfig(const std::string& configJson) {
    try {
        json config = json::parse(configJson);
        
        if (config.contains("topics")) {
            topicKeywords.clear();
            for (const auto& topic : config["topics"].items()) {
                std::vector<std::string> keywords = topic.value().get<std::vector<std::string>>();
                topicKeywords[topic.key()] = keywords;
            }
        }
        
        if (config.contains("contexts")) {
            contextualSituations.clear();
            for (const auto& context : config["contexts"].items()) {
                std::vector<std::string> topics = context.value().get<std::vector<std::string>>();
                contextualSituations[context.key()] = topics;
            }
        }
        
        if (config.contains("synonyms")) {
            synonyms.clear();
            for (const auto& syn : config["synonyms"].items()) {
                std::vector<std::string> words = syn.value().get<std::vector<std::string>>();
                synonyms[syn.key()] = words;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading semantic config: " << e.what() << std::endl;
    }
}

std::string SemanticSearch::exportSemanticConfig() const {
    json config;
    
    config["topics"] = topicKeywords;
    config["contexts"] = contextualSituations;
    config["synonyms"] = synonyms;
    
    return config.dump(2);
}

// Advanced pattern matching methods
std::vector<std::string> SemanticSearch::searchWithWildcards([[maybe_unused]] const std::string& pattern) const {
    std::vector<std::string> results;
    
    // This is a basic wildcard implementation
    // In a full implementation, this would search through all verses
    // For now, return empty as this would need access to verse data
    
    return results;
}

std::vector<std::string> SemanticSearch::searchWithRegex(const std::string& regexPattern) const {
    std::vector<std::string> results;
    
    try {
        std::regex regex(regexPattern, std::regex_constants::icase);
        
        // This would search through all verses with the regex
        // For now, return empty as this would need access to verse data
        
    } catch (const std::regex_error& e) {
        // Invalid regex pattern
    }
    
    return results;
}

bool SemanticSearch::matchesWildcardPattern(const std::string& text, const std::string& pattern) const {
    // Simple wildcard matching: * matches any sequence, ? matches any single character
    std::string regexPattern = pattern;
    
    // Escape special regex characters except * and ?
    std::string escaped = "";
    for (char c : regexPattern) {
        switch (c) {
            case '.': case '^': case '$': case '+': case '{': case '}':
            case '[': case ']': case '(': case ')': case '|': case '\\':
                escaped += "\\";
                escaped += c;
                break;
            case '*':
                escaped += ".*";
                break;
            case '?':
                escaped += ".";
                break;
            default:
                escaped += c;
        }
    }
    
    try {
        std::regex wildcardRegex(escaped, std::regex_constants::icase);
        return std::regex_search(text, wildcardRegex);
    } catch (const std::regex_error& e) {
        return false;
    }
}