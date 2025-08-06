#include "PluginManager.h"
#include <iostream>
#include <fstream>
#include <algorithm>

namespace PluginSystem {

PluginManager::PluginManager(VerseFinder* bible) 
    : api(std::make_unique<PluginAPI>(bible)),
      security(std::make_unique<PluginSecurity>()) {
}

PluginManager::~PluginManager() {
    shutdown();
}

bool PluginManager::initialize(const std::string& pluginsDir, const std::string& configDir) {
    plugins_directory = pluginsDir;
    config_directory = configDir;
    
    // Create directories if they don't exist
    try {
        std::filesystem::create_directories(plugins_directory);
        std::filesystem::create_directories(config_directory);
    } catch (const std::exception& e) {
        last_error = "Failed to create plugin directories: " + std::string(e.what());
        return false;
    }
    
    // Initialize security system
    if (!security->initialize(config_directory)) {
        last_error = "Failed to initialize plugin security";
        return false;
    }
    
    // Scan for available plugins if auto-scan is enabled
    if (auto_scan_enabled) {
        scanForPlugins();
    }
    
    return true;
}

void PluginManager::shutdown() {
    std::lock_guard<std::mutex> lock(plugins_mutex);
    
    // Unload all plugins in reverse order
    std::vector<std::string> plugin_names;
    for (const auto& entry : plugins) {
        plugin_names.push_back(entry.first);
    }
    
    for (auto it = plugin_names.rbegin(); it != plugin_names.rend(); ++it) {
        unloadPlugin(*it);
    }
    
    plugins.clear();
    plugin_metrics.clear();
}

bool PluginManager::loadPlugin(const std::string& pluginName) {
    std::lock_guard<std::mutex> lock(plugins_mutex);
    
    // Check if plugin is already loaded
    auto it = plugins.find(pluginName);
    if (it != plugins.end() && it->second->state != PluginState::UNLOADED) {
        return it->second->state == PluginState::ACTIVE;
    }
    
    // Create or get plugin entry
    if (it == plugins.end()) {
        plugins[pluginName] = std::make_unique<PluginEntry>();
        it = plugins.find(pluginName);
    }
    
    auto& entry = *it->second;
    updatePluginState(pluginName, PluginState::LOADING);
    
    // Load plugin configuration
    if (!loadPluginConfig(pluginName, entry.config)) {
        // Use default config if loading fails
        entry.config = PluginConfig();
        entry.config.pluginDataPath = config_directory + "/" + pluginName;
        entry.config.configFilePath = getPluginConfigPath(pluginName);
    }
    
    // Create plugin loader
    entry.loader = std::make_unique<PluginLoader>();
    
    // Get plugin library path
    std::string libraryPath = plugins_directory + "/" + getPluginLibraryName(pluginName);
    
    // Load the plugin library
    if (!entry.loader->loadPlugin(libraryPath)) {
        std::string error = "Failed to load plugin library: " + entry.loader->getLastError();
        updatePluginState(pluginName, PluginState::ERROR, error);
        triggerLoadCallbacks(pluginName, false, error);
        return false;
    }
    
    // Get the plugin instance
    IPlugin* plugin = entry.loader->getPlugin();
    if (!plugin) {
        std::string error = "Failed to get plugin instance";
        updatePluginState(pluginName, PluginState::ERROR, error);
        triggerLoadCallbacks(pluginName, false, error);
        return false;
    }
    
    // Validate dependencies
    if (!validateDependencies(plugin->getInfo())) {
        std::string error = "Plugin dependencies not satisfied";
        updatePluginState(pluginName, PluginState::ERROR, error);
        triggerLoadCallbacks(pluginName, false, error);
        return false;
    }
    
    // Check security permissions
    if (!security->checkPluginPermissions(pluginName, plugin->getInfo())) {
        std::string error = "Plugin security check failed";
        updatePluginState(pluginName, PluginState::ERROR, error);
        triggerLoadCallbacks(pluginName, false, error);
        return false;
    }
    
    updatePluginState(pluginName, PluginState::LOADED);
    
    // Initialize the plugin
    if (!plugin->initialize()) {
        std::string error = "Plugin initialization failed: " + plugin->getLastError();
        updatePluginState(pluginName, PluginState::ERROR, error);
        triggerLoadCallbacks(pluginName, false, error);
        return false;
    }
    
    // Configure the plugin
    if (!plugin->configure(entry.config)) {
        std::string error = "Plugin configuration failed: " + plugin->getLastError();
        updatePluginState(pluginName, PluginState::ERROR, error);
        triggerLoadCallbacks(pluginName, false, error);
        return false;
    }
    
    // Activate the plugin
    try {
        plugin->onActivate();
        updatePluginState(pluginName, PluginState::ACTIVE);
        entry.load_time = std::chrono::steady_clock::now();
        
        // Trigger plugin loaded event
        PluginEvent event(Events::PLUGIN_LOADED, "PluginManager");
        event.setData("plugin_name", pluginName);
        event.setData("plugin_type", entry.loader->getPluginType());
        triggerEvent(event);
        
        triggerLoadCallbacks(pluginName, true, "");
        return true;
        
    } catch (const std::exception& e) {
        std::string error = "Plugin activation failed: " + std::string(e.what());
        updatePluginState(pluginName, PluginState::ERROR, error);
        triggerLoadCallbacks(pluginName, false, error);
        return false;
    }
}

bool PluginManager::unloadPlugin(const std::string& pluginName) {
    std::lock_guard<std::mutex> lock(plugins_mutex);
    
    auto it = plugins.find(pluginName);
    if (it == plugins.end() || it->second->state == PluginState::UNLOADED) {
        return true;
    }
    
    auto& entry = *it->second;
    updatePluginState(pluginName, PluginState::UNLOADING);
    
    // Get plugin instance
    IPlugin* plugin = entry.loader ? entry.loader->getPlugin() : nullptr;
    
    if (plugin) {
        try {
            // Deactivate plugin
            plugin->onDeactivate();
            
            // Shutdown plugin
            plugin->shutdown();
            
        } catch (const std::exception& e) {
            std::cerr << "Error during plugin shutdown: " << e.what() << std::endl;
        }
    }
    
    // Unload the plugin library
    entry.loader.reset();
    
    updatePluginState(pluginName, PluginState::UNLOADED);
    
    // Trigger plugin unloaded event
    PluginEvent event(Events::PLUGIN_UNLOADED, "PluginManager");
    event.setData("plugin_name", pluginName);
    triggerEvent(event);
    
    triggerUnloadCallbacks(pluginName);
    
    return true;
}

bool PluginManager::reloadPlugin(const std::string& pluginName) {
    if (!unloadPlugin(pluginName)) {
        return false;
    }
    return loadPlugin(pluginName);
}

std::vector<std::string> PluginManager::getLoadedPlugins() const {
    std::lock_guard<std::mutex> lock(plugins_mutex);
    std::vector<std::string> result;
    
    for (const auto& entry : plugins) {
        if (entry.second->state == PluginState::ACTIVE) {
            result.push_back(entry.first);
        }
    }
    
    return result;
}

std::vector<std::string> PluginManager::getAvailablePlugins() const {
    return getPluginFiles(plugins_directory);
}

bool PluginManager::isPluginLoaded(const std::string& pluginName) const {
    std::lock_guard<std::mutex> lock(plugins_mutex);
    auto it = plugins.find(pluginName);
    return it != plugins.end() && it->second->state == PluginState::ACTIVE;
}

PluginState PluginManager::getPluginState(const std::string& pluginName) const {
    std::lock_guard<std::mutex> lock(plugins_mutex);
    auto it = plugins.find(pluginName);
    return it != plugins.end() ? it->second->state : PluginState::UNLOADED;
}

void PluginManager::triggerEvent(const PluginEvent& event) {
    api->triggerEvent(event);
}

// Permission management methods
bool PluginManager::grantPermission(const std::string& pluginName, const std::string& permission) {
    return security->grantPermission(pluginName, permission);
}

bool PluginManager::revokePermission(const std::string& pluginName, const std::string& permission) {
    return security->revokePermission(pluginName, permission);
}

bool PluginManager::hasPermission(const std::string& pluginName, const std::string& permission) {
    return security->hasPermission(pluginName, permission);
}

// Security methods  
bool PluginManager::isPluginTrusted(const std::string& pluginName) const {
    return security->isPluginTrusted(pluginName);
}

bool PluginManager::trustPlugin(const std::string& pluginName) {
    return security->trustPlugin(pluginName);
}

bool PluginManager::untrustPlugin(const std::string& pluginName) {
    return security->untrustPlugin(pluginName);
}

std::vector<std::string> PluginManager::getPluginPermissions(const std::string& pluginName) const {
    // Get available permissions for this plugin
    auto context = security->getContext(pluginName);
    if (context) {
        return context->getGrantedPermissions();
    }
    return {};
}

// Plugin info methods
PluginInfo PluginManager::getPluginInfo(const std::string& pluginName) const {
    std::lock_guard<std::mutex> lock(plugins_mutex);
    auto it = plugins.find(pluginName);
    if (it != plugins.end() && it->second->loader && it->second->loader->getPlugin()) {
        return it->second->loader->getPlugin()->getInfo();
    }
    return PluginInfo(); // Return empty info if not found
}

std::string PluginManager::getPluginError(const std::string& pluginName) const {
    std::lock_guard<std::mutex> lock(plugins_mutex);
    auto it = plugins.find(pluginName);
    return it != plugins.end() ? it->second->error_message : "";
}

// Plugin lifecycle methods
bool PluginManager::enablePlugin(const std::string& pluginName) {
    std::lock_guard<std::mutex> lock(plugins_mutex);
    auto it = plugins.find(pluginName);
    if (it != plugins.end()) {
        it->second->auto_start = true;
        return true;
    }
    return false;
}

bool PluginManager::disablePlugin(const std::string& pluginName) {
    std::lock_guard<std::mutex> lock(plugins_mutex);
    auto it = plugins.find(pluginName);
    if (it != plugins.end()) {
        it->second->auto_start = false;
        return true;
    }
    return false;
}

bool PluginManager::isPluginEnabled(const std::string& pluginName) const {
    std::lock_guard<std::mutex> lock(plugins_mutex);
    auto it = plugins.find(pluginName);
    return it != plugins.end() ? it->second->auto_start : false;
}

void PluginManager::enableAutoStart(const std::string& pluginName, bool enable) {
    std::lock_guard<std::mutex> lock(plugins_mutex);
    auto it = plugins.find(pluginName);
    if (it != plugins.end()) {
        it->second->auto_start = enable;
    }
}

// Performance monitoring
PluginMetrics PluginManager::getPluginMetrics(const std::string& pluginName) const {
    auto it = plugin_metrics.find(pluginName);
    return it != plugin_metrics.end() ? it->second : PluginMetrics();
}

// Installation methods (simplified implementation)
bool PluginManager::installPlugin(const std::string& pluginFile, const std::string& pluginName) {
    // Simple implementation - copy file to plugins directory
    try {
        std::string targetName = pluginName.empty() ? 
            std::filesystem::path(pluginFile).stem().string() : pluginName;
        
        std::string targetPath = plugins_directory + "/" + getPluginLibraryName(targetName);
        std::filesystem::copy_file(pluginFile, targetPath, std::filesystem::copy_options::overwrite_existing);
        
        return true;
    } catch (const std::exception& e) {
        last_error = "Failed to install plugin: " + std::string(e.what());
        return false;
    }
}

bool PluginManager::uninstallPlugin(const std::string& pluginName) {
    // Unload first
    unloadPlugin(pluginName);
    
    // Remove plugin file
    try {
        std::string pluginPath = plugins_directory + "/" + getPluginLibraryName(pluginName);
        std::filesystem::remove(pluginPath);
        
        // Remove from registry
        std::lock_guard<std::mutex> lock(plugins_mutex);
        plugins.erase(pluginName);
        
        return true;
    } catch (const std::exception& e) {
        last_error = "Failed to uninstall plugin: " + std::string(e.what());
        return false;
    }
}

// Helper methods implementation

bool PluginManager::loadPluginConfig(const std::string& pluginName, PluginConfig& config) {
    std::string configPath = getPluginConfigPath(pluginName);
    
    std::ifstream file(configPath);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            config.set(key, value);
        }
    }
    
    return true;
}

bool PluginManager::savePluginConfig(const std::string& pluginName, const PluginConfig& config) {
    std::string configPath = getPluginConfigPath(pluginName);
    
    std::ofstream file(configPath);
    if (!file.is_open()) {
        return false;
    }
    
    for (const auto& setting : config.settings) {
        file << setting.first << "=" << setting.second << std::endl;
    }
    
    return true;
}

bool PluginManager::validateDependencies(const PluginInfo& info) {
    // For now, just check if dependency plugins are loaded
    for (const std::string& dep : info.dependencies) {
        if (!isPluginLoaded(dep)) {
            return false;
        }
    }
    return true;
}

std::vector<std::string> PluginManager::getPluginFiles(const std::string& directory) const {
    std::vector<std::string> result;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                std::string extension = entry.path().extension().string();
                
#ifdef _WIN32
                if (extension == ".dll") {
#elif __APPLE__
                if (extension == ".dylib") {
#else
                if (extension == ".so") {
#endif
                    // Remove extension to get plugin name
                    std::string pluginName = filename.substr(0, filename.find_last_of('.'));
                    result.push_back(pluginName);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning plugin directory: " << e.what() << std::endl;
    }
    
    return result;
}

std::string PluginManager::getPluginConfigPath(const std::string& pluginName) {
    return config_directory + "/" + pluginName + ".conf";
}

std::string PluginManager::getPluginLibraryName(const std::string& pluginName) {
#ifdef _WIN32
    return pluginName + ".dll";
#elif __APPLE__
    return "lib" + pluginName + ".dylib";
#else
    return "lib" + pluginName + ".so";
#endif
}

void PluginManager::updatePluginState(const std::string& pluginName, PluginState state, const std::string& error) {
    auto it = plugins.find(pluginName);
    if (it != plugins.end()) {
        it->second->state = state;
        it->second->error_message = error;
        it->second->last_activity = std::chrono::steady_clock::now();
    }
}

void PluginManager::triggerLoadCallbacks(const std::string& pluginName, bool success, const std::string& error) {
    for (const auto& callback : load_callbacks) {
        try {
            callback(pluginName, success, error);
        } catch (...) {
            // Ignore callback errors
        }
    }
}

void PluginManager::triggerUnloadCallbacks(const std::string& pluginName) {
    for (const auto& callback : unload_callbacks) {
        try {
            callback(pluginName);
        } catch (...) {
            // Ignore callback errors
        }
    }
}

bool PluginManager::scanForPlugins() {
    std::vector<std::string> available = getAvailablePlugins();
    
    for (const std::string& pluginName : available) {
        std::lock_guard<std::mutex> lock(plugins_mutex);
        
        // Create entry if it doesn't exist
        if (plugins.find(pluginName) == plugins.end()) {
            plugins[pluginName] = std::make_unique<PluginEntry>();
        }
        
        // Auto-load plugins marked for auto-start
        auto& entry = *plugins[pluginName];
        if (entry.auto_start && entry.state == PluginState::UNLOADED) {
            // Unlock for loading (loadPlugin will acquire its own lock)
            plugins_mutex.unlock();
            loadPlugin(pluginName);
            plugins_mutex.lock();
        }
    }
    
    return true;
}

void PluginManager::addLoadCallback(PluginLoadCallback callback) {
    load_callbacks.push_back(callback);
}

void PluginManager::addUnloadCallback(PluginUnloadCallback callback) {
    unload_callbacks.push_back(callback);
}

void PluginManager::update(float deltaTime) {
    std::lock_guard<std::mutex> lock(plugins_mutex);
    
    for (const auto& entry : plugins) {
        if (entry.second->state == PluginState::ACTIVE) {
            IPlugin* plugin = entry.second->loader->getPlugin();
            if (plugin) {
                try {
                    auto start = std::chrono::high_resolution_clock::now();
                    plugin->onUpdate(deltaTime);
                    auto end = std::chrono::high_resolution_clock::now();
                    
                    if (performance_monitoring_enabled) {
                        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                        recordPluginCall(entry.first, duration);
                    }
                } catch (const std::exception& e) {
                    recordPluginError(entry.first);
                    std::cerr << "Plugin update error in " << entry.first << ": " << e.what() << std::endl;
                }
            }
        }
    }
}

void PluginManager::recordPluginCall(const std::string& pluginName, std::chrono::microseconds execution_time) {
    plugin_metrics[pluginName].recordCall(execution_time);
}

void PluginManager::recordPluginError(const std::string& pluginName) {
    plugin_metrics[pluginName].recordError();
}

} // namespace PluginSystem