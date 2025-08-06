#include "interfaces/PluginInterfaces.h"
#include "api/PluginAPI.h"

using namespace PluginSystem;

class SimpleUIPlugin : public IUIPlugin {
private:
    PluginInfo info;
    PluginState state;
    PluginAPI* api;
    std::string last_error;
    
public:
    SimpleUIPlugin() : state(PluginState::UNLOADED), api(nullptr) {
        info.name = "Simple UI Plugin";
        info.description = "A simple example UI plugin that adds a custom menu item";
        info.author = "VerseFinder SDK";
        info.version = {1, 0, 0};
        info.website = "https://versefinder.com/plugins/simple-ui";
        info.tags = {"ui", "example", "menu"};
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
        // Configuration can include UI preferences, colors, etc.
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
    
    // IUIPlugin interface
    void addMenuItems() override {
        // This would add menu items to the application
        // In a real implementation, this would call ImGui functions
    }
    
    void removeMenuItems() override {
        // Remove menu items when plugin is deactivated
    }
    
    void renderCustomPanel() override {
        // Render a custom panel in the main UI
        // Example ImGui code would go here
    }
    
    void renderSettings() override {
        // Render plugin-specific settings
        // Example ImGui settings controls would go here
    }
    
    // UI capabilities
    bool hasCustomPanel() const override {
        return true;
    }
    
    bool hasMenuItems() const override {
        return true;
    }
    
    bool hasSettings() const override {
        return true;
    }
    
    std::string getUIDescription() const override {
        return "Adds a simple custom panel and menu items for demonstration purposes";
    }
    
    void setAPI(PluginAPI* pluginAPI) {
        api = pluginAPI;
    }
};

// Plugin factory functions
extern "C" {
    PluginSystem::IPlugin* createPlugin() {
        return new SimpleUIPlugin();
    }
    
    void destroyPlugin(PluginSystem::IPlugin* plugin) {
        delete plugin;
    }
    
    const char* getPluginApiVersion() {
        return "1.0";
    }
    
    const char* getPluginType() {
        return "ui";
    }
}