#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include "../interfaces/PluginInterfaces.h"
#include "../api/PluginAPI.h"
#include "../loader/PluginLoader.h"
#include "../security/PluginSecurity.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <filesystem>

namespace PluginSystem {

// Plugin registry entry
struct PluginEntry {
    std::unique_ptr<PluginLoader> loader;
    PluginConfig config;
    PluginState state;
    std::string error_message;
    std::chrono::steady_clock::time_point load_time;
    std::chrono::steady_clock::time_point last_activity;
    bool auto_start = true;
    
    PluginEntry() : state(PluginState::UNLOADED) {}
};

// Plugin dependency information
struct PluginDependency {
    std::string name;
    PluginVersion min_version;
    PluginVersion max_version;
    bool required = true;
};

// Plugin performance metrics
struct PluginMetrics {
    std::chrono::microseconds total_execution_time{0};
    size_t call_count = 0;
    size_t error_count = 0;
    double average_execution_time_ms = 0.0;
    std::chrono::steady_clock::time_point last_call;
    
    void recordCall(std::chrono::microseconds execution_time) {
        total_execution_time += execution_time;
        call_count++;
        average_execution_time_ms = static_cast<double>(total_execution_time.count()) / (call_count * 1000.0);
        last_call = std::chrono::steady_clock::now();
    }
    
    void recordError() {
        error_count++;
    }
};

// Plugin manager callbacks
using PluginLoadCallback = std::function<void(const std::string& pluginName, bool success, const std::string& error)>;
using PluginUnloadCallback = std::function<void(const std::string& pluginName)>;

class PluginManager {
private:
    std::unordered_map<std::string, std::unique_ptr<PluginEntry>> plugins;
    std::unordered_map<std::string, PluginMetrics> plugin_metrics;
    std::unique_ptr<PluginAPI> api;
    std::unique_ptr<PluginSecurity> security;
    mutable std::mutex plugins_mutex;
    std::string plugins_directory;
    std::string config_directory;
    bool auto_scan_enabled = true;
    
    // Callbacks
    std::vector<PluginLoadCallback> load_callbacks;
    std::vector<PluginUnloadCallback> unload_callbacks;
    
    // Helper methods
    bool loadPluginConfig(const std::string& pluginName, PluginConfig& config);
    bool savePluginConfig(const std::string& pluginName, const PluginConfig& config);
    bool validateDependencies(const PluginInfo& info);
    std::vector<std::string> getPluginFiles(const std::string& directory) const;
    std::string getPluginConfigPath(const std::string& pluginName);
    std::string getPluginLibraryName(const std::string& pluginName);
    void updatePluginState(const std::string& pluginName, PluginState state, const std::string& error = "");
    void triggerLoadCallbacks(const std::string& pluginName, bool success, const std::string& error);
    void triggerUnloadCallbacks(const std::string& pluginName);
    
public:
    explicit PluginManager(VerseFinder* bible);
    ~PluginManager();
    
    // Initialization and configuration
    bool initialize(const std::string& pluginsDir, const std::string& configDir);
    void shutdown();
    
    // Plugin loading and unloading
    bool loadPlugin(const std::string& pluginName);
    bool loadPluginFromFile(const std::string& filePath);
    bool unloadPlugin(const std::string& pluginName);
    bool reloadPlugin(const std::string& pluginName);
    
    // Plugin management
    std::vector<std::string> getLoadedPlugins() const;
    std::vector<std::string> getAvailablePlugins() const;
    std::vector<std::string> getPluginsByType(const std::string& type) const;
    bool isPluginLoaded(const std::string& pluginName) const;
    PluginState getPluginState(const std::string& pluginName) const;
    std::string getPluginError(const std::string& pluginName) const;
    
    // Plugin information
    PluginInfo getPluginInfo(const std::string& pluginName) const;
    std::vector<PluginInfo> getAllPluginInfo() const;
    std::string getPluginType(const std::string& pluginName) const;
    
    // Plugin configuration
    bool configurePlugin(const std::string& pluginName, const PluginConfig& config);
    PluginConfig getPluginConfig(const std::string& pluginName) const;
    bool setPluginSetting(const std::string& pluginName, const std::string& key, const std::string& value);
    std::string getPluginSetting(const std::string& pluginName, const std::string& key, const std::string& defaultValue = "") const;
    
    // Plugin discovery and installation
    bool scanForPlugins();
    bool installPlugin(const std::string& pluginFile, const std::string& pluginName = "");
    bool uninstallPlugin(const std::string& pluginName);
    
    // Plugin lifecycle management
    bool enablePlugin(const std::string& pluginName);
    bool disablePlugin(const std::string& pluginName);
    bool isPluginEnabled(const std::string& pluginName) const;
    void enableAutoStart(const std::string& pluginName, bool enable);
    
    // Plugin execution and access
    template<typename T>
    T* getPlugin(const std::string& pluginName) const {
        std::lock_guard<std::mutex> lock(plugins_mutex);
        auto it = plugins.find(pluginName);
        if (it != plugins.end() && it->second->state == PluginState::ACTIVE) {
            return dynamic_cast<T*>(it->second->loader->getPlugin());
        }
        return nullptr;
    }
    
    template<typename T>
    std::vector<T*> getPluginsByType() const {
        std::vector<T*> result;
        std::lock_guard<std::mutex> lock(plugins_mutex);
        
        for (const auto& entry : plugins) {
            if (entry.second->state == PluginState::ACTIVE) {
                if (auto plugin = dynamic_cast<T*>(entry.second->loader->getPlugin())) {
                    result.push_back(plugin);
                }
            }
        }
        return result;
    }
    
    // Event system
    void triggerEvent(const PluginEvent& event);
    
    // Performance monitoring
    PluginMetrics getPluginMetrics(const std::string& pluginName) const;
    std::vector<std::pair<std::string, PluginMetrics>> getAllPluginMetrics() const;
    void resetPluginMetrics(const std::string& pluginName);
    void enablePerformanceMonitoring(bool enable);
    
    // Security
    bool isPluginTrusted(const std::string& pluginName) const;
    bool trustPlugin(const std::string& pluginName);
    bool untrustPlugin(const std::string& pluginName);
    std::vector<std::string> getPluginPermissions(const std::string& pluginName) const;
    bool grantPermission(const std::string& pluginName, const std::string& permission);
    bool revokePermission(const std::string& pluginName, const std::string& permission);
    bool hasPermission(const std::string& pluginName, const std::string& permission);
    
    // Callbacks
    void addLoadCallback(PluginLoadCallback callback);
    void addUnloadCallback(PluginUnloadCallback callback);
    
    // Auto-scanning
    void enableAutoScan(bool enable);
    bool isAutoScanEnabled() const;
    
    // Directories
    std::string getPluginsDirectory() const { return plugins_directory; }
    std::string getConfigDirectory() const { return config_directory; }
    
    // API access
    PluginAPI* getAPI() const { return api.get(); }
    
    // Update handling
    void update(float deltaTime);
    
    // Error handling
    std::string getLastError() const;
    void clearErrors();
    
private:
    std::string last_error;
    bool performance_monitoring_enabled = true;
    
    void recordPluginCall(const std::string& pluginName, std::chrono::microseconds execution_time);
    void recordPluginError(const std::string& pluginName);
};

} // namespace PluginSystem

#endif // PLUGIN_MANAGER_H