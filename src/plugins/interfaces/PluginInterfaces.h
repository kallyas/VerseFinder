#ifndef PLUGIN_INTERFACES_H
#define PLUGIN_INTERFACES_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

// Forward declarations
class VerseFinder;
struct SearchResult;

namespace PluginSystem {

// Configuration data for plugins
struct PluginConfig {
    std::unordered_map<std::string, std::string> settings;
    std::string pluginDataPath;
    std::string configFilePath;
    bool debugMode = false;
    
    std::string getString(const std::string& key, const std::string& defaultValue = "") const {
        auto it = settings.find(key);
        return it != settings.end() ? it->second : defaultValue;
    }
    
    int getInt(const std::string& key, int defaultValue = 0) const {
        auto it = settings.find(key);
        if (it != settings.end()) {
            try {
                return std::stoi(it->second);
            } catch (...) {
                return defaultValue;
            }
        }
        return defaultValue;
    }
    
    bool getBool(const std::string& key, bool defaultValue = false) const {
        auto it = settings.find(key);
        if (it != settings.end()) {
            return it->second == "true" || it->second == "1" || it->second == "yes";
        }
        return defaultValue;
    }
    
    void set(const std::string& key, const std::string& value) {
        settings[key] = value;
    }
};

// Plugin version information
struct PluginVersion {
    int major = 1;
    int minor = 0;
    int patch = 0;
    
    std::string toString() const {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }
};

// Plugin metadata
struct PluginInfo {
    std::string name;
    std::string description;
    std::string author;
    PluginVersion version;
    std::string website;
    std::vector<std::string> dependencies;
    std::vector<std::string> tags;
    bool enabled = true;
};

// Plugin lifecycle states
enum class PluginState {
    UNLOADED,
    LOADING,
    LOADED,
    ACTIVE,
    ERROR,
    UNLOADING
};

// Base plugin interface - all plugins must implement this
class IPlugin {
public:
    virtual ~IPlugin() = default;
    
    // Required lifecycle methods
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual const PluginInfo& getInfo() const = 0;
    
    // Optional lifecycle methods
    virtual bool configure(const PluginConfig& config) { return true; }
    virtual void onActivate() {}
    virtual void onDeactivate() {}
    virtual void onUpdate(float deltaTime) {}
    
    // Plugin state
    virtual PluginState getState() const = 0;
    virtual std::string getLastError() const { return ""; }
};

// Search algorithm plugin interface
class ISearchPlugin : public IPlugin {
public:
    virtual ~ISearchPlugin() = default;
    
    // Search methods
    virtual std::vector<std::string> search(const std::string& query, const std::string& translation) = 0;
    virtual std::vector<std::string> searchAdvanced(const std::string& query, 
                                                   const std::string& translation,
                                                   const std::unordered_map<std::string, std::string>& options) = 0;
    
    // Search capabilities
    virtual bool supportsTranslation(const std::string& translation) const = 0;
    virtual std::vector<std::string> getSupportedOptions() const = 0;
    virtual std::string getSearchDescription() const = 0;
    virtual double getSearchQuality(const std::string& query) const = 0; // 0.0-1.0 quality score
};

// UI extension plugin interface
class IUIPlugin : public IPlugin {
public:
    virtual ~IUIPlugin() = default;
    
    // UI extension points
    virtual void addMenuItems() = 0;
    virtual void removeMenuItems() = 0;
    virtual void renderCustomPanel() = 0;
    virtual void renderSettings() = 0;
    
    // UI capabilities
    virtual bool hasCustomPanel() const = 0;
    virtual bool hasMenuItems() const = 0;
    virtual bool hasSettings() const = 0;
    virtual std::string getUIDescription() const = 0;
};

// Translation format parser plugin interface
class ITranslationPlugin : public IPlugin {
public:
    virtual ~ITranslationPlugin() = default;
    
    // Translation parsing
    virtual bool canParse(const std::string& filename) const = 0;
    virtual bool parseFile(const std::string& filename, VerseFinder* bible) = 0;
    virtual bool parseData(const std::string& data, VerseFinder* bible) = 0;
    
    // Format information
    virtual std::vector<std::string> getSupportedExtensions() const = 0;
    virtual std::string getFormatDescription() const = 0;
    virtual bool supportsExport() const = 0;
    virtual bool exportData(const std::string& filename, VerseFinder* bible) = 0;
};

// Theme engine plugin interface  
class IThemePlugin : public IPlugin {
public:
    virtual ~IThemePlugin() = default;
    
    // Theme management
    virtual void applyTheme() = 0;
    virtual void resetTheme() = 0;
    virtual bool loadThemeFromFile(const std::string& filename) = 0;
    virtual bool saveThemeToFile(const std::string& filename) = 0;
    
    // Theme properties
    virtual std::string getThemeName() const = 0;
    virtual std::string getThemeDescription() const = 0;
    virtual std::vector<std::string> getCustomizableProperties() const = 0;
    virtual bool setProperty(const std::string& property, const std::string& value) = 0;
    virtual std::string getProperty(const std::string& property) const = 0;
};

// Integration connector plugin interface
class IIntegrationPlugin : public IPlugin {
public:
    virtual ~IIntegrationPlugin() = default;
    
    // Integration methods
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual bool testConnection() = 0;
    
    // Data exchange
    virtual bool sendVerse(const std::string& verse, const std::string& reference) = 0;
    virtual bool sendServicePlan(const std::string& planData) = 0;
    virtual std::string receiveData() = 0;
    
    // Integration info
    virtual std::string getServiceName() const = 0;
    virtual std::string getConnectionStatus() const = 0;
    virtual std::vector<std::string> getSupportedFeatures() const = 0;
};

// Export format converter plugin interface
class IExportPlugin : public IPlugin {
public:
    virtual ~IExportPlugin() = default;
    
    // Export methods
    virtual bool exportVerse(const std::string& verse, const std::string& reference, 
                           const std::string& filename) = 0;
    virtual bool exportVerses(const std::vector<std::string>& verses, 
                            const std::vector<std::string>& references,
                            const std::string& filename) = 0;
    virtual bool exportServicePlan(const std::string& planData, const std::string& filename) = 0;
    
    // Format info
    virtual std::string getFormatName() const = 0;
    virtual std::string getFileExtension() const = 0;
    virtual std::vector<std::string> getSupportedOptions() const = 0;
    virtual bool supportsMultipleVerses() const = 0;
};

// Scripting engine plugin interface
class IScriptPlugin : public IPlugin {
public:
    virtual ~IScriptPlugin() = default;
    
    // Script execution
    virtual bool executeScript(const std::string& script) = 0;
    virtual bool executeScriptFile(const std::string& filename) = 0;
    virtual bool validateScript(const std::string& script) = 0;
    
    // Script management
    virtual std::string getScriptLanguage() const = 0;
    virtual std::vector<std::string> getAvailableFunctions() const = 0;
    virtual void registerFunction(const std::string& name, void* function) = 0;
    virtual void setVariable(const std::string& name, const std::string& value) = 0;
    virtual std::string getVariable(const std::string& name) const = 0;
};

} // namespace PluginSystem

// Plugin factory function type
typedef PluginSystem::IPlugin* (*CreatePluginFunc)();
typedef void (*DestroyPluginFunc)(PluginSystem::IPlugin*);

// Required export functions for plugins
extern "C" {
    // Every plugin must export these functions
    PluginSystem::IPlugin* createPlugin();
    void destroyPlugin(PluginSystem::IPlugin* plugin);
    const char* getPluginApiVersion();
    const char* getPluginType();
}

#endif // PLUGIN_INTERFACES_H