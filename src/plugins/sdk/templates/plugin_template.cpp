#include "interfaces/PluginInterfaces.h"
#include "api/PluginAPI.h"

using namespace PluginSystem;

class {{PLUGIN_CLASS_NAME}} : public I{{PLUGIN_TYPE}}Plugin {
private:
    PluginInfo info;
    PluginState state;
    PluginAPI* api;
    std::string last_error;
    
public:
    {{PLUGIN_CLASS_NAME}}() : state(PluginState::UNLOADED), api(nullptr) {
        info.name = "{{PLUGIN_NAME}}";
        info.description = "{{PLUGIN_DESCRIPTION}}";
        info.author = "{{PLUGIN_AUTHOR}}";
        info.version = {1, 0, 0};
        info.website = "{{PLUGIN_WEBSITE}}";
        info.tags = {"{{PLUGIN_TAGS}}"};
    }
    
    // IPlugin interface implementation
    bool initialize() override {
        try {
            // TODO: Add your initialization code here
            state = PluginState::LOADED;
            return true;
        } catch (const std::exception& e) {
            last_error = "Initialization failed: " + std::string(e.what());
            state = PluginState::ERROR;
            return false;
        }
    }
    
    void shutdown() override {
        // TODO: Add your cleanup code here
        state = PluginState::UNLOADED;
        api = nullptr;
    }
    
    const PluginInfo& getInfo() const override {
        return info;
    }
    
    bool configure(const PluginConfig& config) override {
        // TODO: Add your configuration code here
        // Example:
        // auto setting = config.getString("my_setting", "default_value");
        return true;
    }
    
    void onActivate() override {
        state = PluginState::ACTIVE;
        // TODO: Add activation code here
    }
    
    void onDeactivate() override {
        state = PluginState::LOADED;
        // TODO: Add deactivation code here
    }
    
    void onUpdate(float deltaTime) override {
        // TODO: Add update logic here (called every frame)
        // Remove this override if not needed
    }
    
    PluginState getState() const override {
        return state;
    }
    
    std::string getLastError() const override {
        return last_error;
    }
    
    // I{{PLUGIN_TYPE}}Plugin interface implementation
    {{PLUGIN_INTERFACE_METHODS}}
    
    // Utility method to set API reference
    void setAPI(PluginAPI* pluginAPI) {
        api = pluginAPI;
    }

private:
    // TODO: Add your private methods here
};

// Required plugin factory functions
extern "C" {
    PluginSystem::IPlugin* createPlugin() {
        return new {{PLUGIN_CLASS_NAME}}();
    }
    
    void destroyPlugin(PluginSystem::IPlugin* plugin) {
        delete plugin;
    }
    
    const char* getPluginApiVersion() {
        return "1.0";
    }
    
    const char* getPluginType() {
        return "{{PLUGIN_TYPE_LOWER}}";
    }
}