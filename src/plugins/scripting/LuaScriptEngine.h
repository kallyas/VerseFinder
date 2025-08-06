#ifndef LUA_SCRIPT_ENGINE_H
#define LUA_SCRIPT_ENGINE_H

#include "../interfaces/PluginInterfaces.h"
#include "../api/PluginAPI.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

// Forward declaration for Lua state
struct lua_State;

namespace PluginSystem {

// Lua script wrapper
class LuaScript {
private:
    std::string script_content;
    std::string script_name;
    bool is_compiled;
    
public:
    LuaScript(const std::string& name, const std::string& content)
        : script_name(name), script_content(content), is_compiled(false) {}
    
    const std::string& getName() const { return script_name; }
    const std::string& getContent() const { return script_content; }
    bool isCompiled() const { return is_compiled; }
    void setCompiled(bool compiled) { is_compiled = compiled; }
};

// Lua function callback type
using LuaFunction = std::function<int(lua_State*)>;

// Lua script engine
class LuaScriptEngine : public IScriptPlugin {
private:
    lua_State* lua_state;
    PluginAPI* api;
    PluginInfo plugin_info;
    PluginState current_state;
    std::string last_error;
    
    std::unordered_map<std::string, LuaFunction> registered_functions;
    std::unordered_map<std::string, std::unique_ptr<LuaScript>> loaded_scripts;
    
    // Lua C functions for API binding
    static int lua_searchByReference(lua_State* L);
    static int lua_searchByKeywords(lua_State* L);
    static int lua_addToFavorites(lua_State* L);
    static int lua_getRandomVerse(lua_State* L);
    static int lua_getVerseOfTheDay(lua_State* L);
    static int lua_triggerEvent(lua_State* L);
    static int lua_log(lua_State* L);
    
    // Helper methods
    void registerCoreFunctions();
    void registerUtilityFunctions();
    bool loadLuaLibraries();
    void setupErrorHandling();
    std::string getLuaError();
    
public:
    LuaScriptEngine();
    ~LuaScriptEngine() override;
    
    // IPlugin interface
    bool initialize() override;
    void shutdown() override;
    const PluginInfo& getInfo() const override;
    bool configure(const PluginConfig& config) override;
    void onActivate() override;
    void onDeactivate() override;
    void onUpdate(float deltaTime) override;
    PluginState getState() const override;
    std::string getLastError() const override;
    
    // IScriptPlugin interface
    bool executeScript(const std::string& script) override;
    bool executeScriptFile(const std::string& filename) override;
    bool validateScript(const std::string& script) override;
    std::string getScriptLanguage() const override;
    std::vector<std::string> getAvailableFunctions() const override;
    void registerFunction(const std::string& name, void* function) override;
    void setVariable(const std::string& name, const std::string& value) override;
    std::string getVariable(const std::string& name) const override;
    
    // Lua-specific methods
    void setAPI(PluginAPI* pluginAPI) { api = pluginAPI; }
    bool loadScript(const std::string& name, const std::string& content);
    bool unloadScript(const std::string& name);
    std::vector<std::string> getLoadedScripts() const;
    bool callLuaFunction(const std::string& functionName, const std::vector<std::string>& args);
    
    // Script management
    bool saveScript(const std::string& name, const std::string& filename);
    bool loadScriptFromFile(const std::string& filename);
    void clearAllScripts();
    
    // Error handling
    void setErrorHandler(std::function<void(const std::string&)> handler);
    
private:
    std::function<void(const std::string&)> error_handler;
};

// Lua utility functions
namespace LuaUtils {
    // Stack manipulation helpers
    void pushString(lua_State* L, const std::string& str);
    void pushNumber(lua_State* L, double num);
    void pushBoolean(lua_State* L, bool value);
    void pushStringArray(lua_State* L, const std::vector<std::string>& arr);
    
    std::string toString(lua_State* L, int index);
    double toNumber(lua_State* L, int index);
    bool toBoolean(lua_State* L, int index);
    std::vector<std::string> toStringArray(lua_State* L, int index);
    
    // Type checking
    bool isString(lua_State* L, int index);
    bool isNumber(lua_State* L, int index);
    bool isBoolean(lua_State* L, int index);
    bool isTable(lua_State* L, int index);
    bool isFunction(lua_State* L, int index);
    
    // Error handling
    int errorHandler(lua_State* L);
    std::string getStackTrace(lua_State* L);
}

// Sample Lua scripts for common operations
namespace SampleScripts {
    constexpr const char* VERSE_OF_THE_DAY = R"(
        -- Get a random verse for the verse of the day
        function getVerseOfTheDay()
            local verse = getRandomVerse()
            log("Selected verse of the day: " .. verse)
            return verse
        end
        
        -- Schedule verse of the day
        function scheduleVerseOfTheDay()
            local verse = getVerseOfTheDay()
            triggerEvent("verse_of_the_day", "verse", verse)
            return verse
        end
    )";
    
    constexpr const char* SEARCH_HELPER = R"(
        -- Enhanced search function
        function enhancedSearch(query, translation)
            translation = translation or "KJV"
            
            -- Try exact reference first
            local result = searchByReference(query, translation)
            if result ~= "" then
                return {result}
            end
            
            -- Try keyword search
            local results = searchByKeywords(query, translation)
            if #results > 0 then
                return results
            end
            
            -- Return empty if nothing found
            return {}
        end
        
        -- Smart search with suggestions
        function smartSearch(query, translation)
            local results = enhancedSearch(query, translation)
            
            if #results == 0 then
                log("No results found for: " .. query)
                -- Could add fuzzy search suggestions here
            else
                log("Found " .. #results .. " results for: " .. query)
            end
            
            return results
        end
    )";
    
    constexpr const char* AUTO_FAVORITE = R"(
        -- Automatically add popular verses to favorites
        function autoAddPopularToFavorites()
            local popular = {
                "John 3:16",
                "Philippians 4:13",
                "Romans 8:28",
                "Jeremiah 29:11",
                "Psalm 23:1"
            }
            
            for i, reference in ipairs(popular) do
                local verse = searchByReference(reference, "KJV")
                if verse ~= "" then
                    addToFavorites(reference)
                    log("Added to favorites: " .. reference)
                end
            end
        end
    )";
    
    constexpr const char* EVENT_LOGGER = R"(
        -- Log all events for debugging
        function logEvent(eventType, data)
            log("Event: " .. eventType .. " Data: " .. tostring(data))
        end
        
        -- Setup event listeners
        function setupEventLogging()
            -- This would need event system integration
            log("Event logging initialized")
        end
    )";
}

} // namespace PluginSystem

#endif // LUA_SCRIPT_ENGINE_H