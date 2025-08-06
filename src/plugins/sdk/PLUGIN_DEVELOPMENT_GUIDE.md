# VerseFinder Plugin SDK

Welcome to the VerseFinder Plugin SDK! This guide will help you create custom plugins to extend VerseFinder's functionality.

## Quick Start

### 1. Plugin Types

VerseFinder supports several types of plugins:

- **Search Plugins** (`ISearchPlugin`): Custom search algorithms
- **UI Plugins** (`IUIPlugin`): User interface extensions  
- **Translation Plugins** (`ITranslationPlugin`): Bible format parsers
- **Theme Plugins** (`IThemePlugin`): Custom appearance themes
- **Integration Plugins** (`IIntegrationPlugin`): External service connectors
- **Export Plugins** (`IExportPlugin`): Output format converters
- **Script Plugins** (`IScriptPlugin`): Scripting engine support

### 2. Basic Plugin Structure

```cpp
#include "interfaces/PluginInterfaces.h"
#include "api/PluginAPI.h"

using namespace PluginSystem;

class MyPlugin : public ISearchPlugin {  // or other interface
private:
    PluginInfo info;
    PluginState state;
    PluginAPI* api;
    
public:
    MyPlugin() : state(PluginState::UNLOADED), api(nullptr) {
        info.name = "My Plugin";
        info.description = "Description of what the plugin does";
        info.author = "Your Name";
        info.version = {1, 0, 0};
        info.tags = {"search", "example"};
    }
    
    // Required IPlugin methods
    bool initialize() override {
        state = PluginState::LOADED;
        return true;
    }
    
    void shutdown() override {
        state = PluginState::UNLOADED;
    }
    
    const PluginInfo& getInfo() const override {
        return info;
    }
    
    PluginState getState() const override {
        return state;
    }
    
    // Plugin-specific interface methods here...
};

// Required exports
extern "C" {
    PluginSystem::IPlugin* createPlugin() {
        return new MyPlugin();
    }
    
    void destroyPlugin(PluginSystem::IPlugin* plugin) {
        delete plugin;
    }
    
    const char* getPluginApiVersion() {
        return "1.0";
    }
    
    const char* getPluginType() {
        return "search";  // or "ui", "translation", etc.
    }
}
```

### 3. Build Configuration

Create a `CMakeLists.txt` for your plugin:

```cmake
cmake_minimum_required(VERSION 3.16)
project(my_plugin)

set(CMAKE_CXX_STANDARD 20)

# Set VerseFinder path
set(VERSEFINDER_ROOT "/path/to/VerseFinder")

include_directories(
    ${VERSEFINDER_ROOT}/src/plugins/interfaces
    ${VERSEFINDER_ROOT}/src/plugins/api
    ${VERSEFINDER_ROOT}/src/core
    ${VERSEFINDER_ROOT}/src/plugins
)

add_library(my_plugin SHARED my_plugin.cpp)

# Platform-specific naming
if(WIN32)
    set_target_properties(my_plugin PROPERTIES
        OUTPUT_NAME "my_plugin"
        PREFIX ""
        SUFFIX ".dll"
    )
elseif(APPLE)
    set_target_properties(my_plugin PROPERTIES
        PREFIX "lib"
        SUFFIX ".dylib"
    )
else()
    set_target_properties(my_plugin PROPERTIES
        PREFIX "lib"
        SUFFIX ".so"
    )
endif()
```

### 4. Using the Plugin API

The `PluginAPI` class provides access to VerseFinder's functionality:

```cpp
void MyPlugin::onActivate() {
    state = PluginState::ACTIVE;
    
    // Search for verses
    auto results = api->searchByKeywords("love", "KJV");
    
    // Access translations
    auto translations = api->getTranslations();
    
    // Add to favorites
    api->addToFavorites("John 3:16");
    
    // Trigger events
    PluginEvent event("my_plugin_activated", "MyPlugin");
    event.setData("timestamp", std::to_string(time(nullptr)));
    api->triggerEvent(event);
}
```

## Plugin Interfaces

### ISearchPlugin

Implement custom search algorithms:

```cpp
class MySearchPlugin : public ISearchPlugin {
public:
    std::vector<std::string> search(const std::string& query, 
                                   const std::string& translation) override {
        // Your search implementation
        return {};
    }
    
    std::vector<std::string> searchAdvanced(const std::string& query,
                                            const std::string& translation,
                                            const std::unordered_map<std::string, std::string>& options) override {
        // Advanced search with options
        return {};
    }
    
    bool supportsTranslation(const std::string& translation) const override {
        return true; // Support all translations
    }
    
    double getSearchQuality(const std::string& query) const override {
        return 0.8; // Quality score 0.0-1.0
    }
};
```

### IUIPlugin

Add custom UI elements:

```cpp
class MyUIPlugin : public IUIPlugin {
public:
    void addMenuItems() override {
        // Add menu items to VerseFinder
    }
    
    void renderCustomPanel() override {
        // Render your custom UI panel using ImGui
        if (ImGui::Begin("My Plugin Panel")) {
            ImGui::Text("Hello from my plugin!");
            if (ImGui::Button("Do Something")) {
                // Plugin action
            }
        }
        ImGui::End();
    }
    
    bool hasCustomPanel() const override { return true; }
    bool hasMenuItems() const override { return true; }
};
```

### ITranslationPlugin

Parse custom Bible formats:

```cpp
class MyFormatPlugin : public ITranslationPlugin {
public:
    bool canParse(const std::string& filename) const override {
        return filename.ends_with(".mybible");
    }
    
    bool parseFile(const std::string& filename, VerseFinder* bible) override {
        // Parse your custom format and populate VerseFinder
        return true;
    }
    
    std::vector<std::string> getSupportedExtensions() const override {
        return {".mybible", ".custom"};
    }
};
```

## Security and Permissions

### Request Permissions

Plugins run in a sandboxed environment. Request only the permissions you need:

```cpp
// Available permissions:
// - bible.read: Read Bible data
// - bible.write: Modify Bible data  
// - file.read: Read files
// - file.write: Write files
// - network.access: Internet access
// - ui.modify: Modify user interface
// - settings.read: Read application settings
// - settings.write: Modify settings
// - presentation.control: Control presentation mode
```

The plugin manager handles permission granting through the UI.

### Resource Limits

Plugins are subject to resource limits:
- Memory: 100MB default
- CPU time: 1 second per operation
- Network requests: 100 per minute
- Disk I/O: 50MB per minute

## Best Practices

### 1. Error Handling

Always handle errors gracefully:

```cpp
bool MyPlugin::initialize() {
    try {
        // Plugin initialization
        state = PluginState::LOADED;
        return true;
    } catch (const std::exception& e) {
        last_error = "Initialization failed: " + std::string(e.what());
        state = PluginState::ERROR;
        return false;
    }
}
```

### 2. Configuration

Use the PluginConfig for settings:

```cpp
bool MyPlugin::configure(const PluginConfig& config) {
    auto setting = config.getString("my_setting", "default_value");
    auto number = config.getInt("my_number", 42);
    auto flag = config.getBool("my_flag", false);
    return true;
}
```

### 3. Event Handling

Listen for and emit events:

```cpp
void MyPlugin::onActivate() {
    // Listen for events
    api->addEventListener("verse_selected", [this](const PluginEvent& event) {
        auto verse = event.getData("verse");
        // Handle verse selection
    });
    
    // Emit events
    PluginEvent event("my_custom_event", getInfo().name);
    event.setData("message", "Hello from plugin");
    api->triggerEvent(event);
}
```

### 4. Cleanup

Always clean up resources:

```cpp
void MyPlugin::shutdown() {
    // Remove event listeners
    api->removeEventListener("verse_selected");
    
    // Clean up resources
    // ...
    
    state = PluginState::UNLOADED;
}
```

## Installation

1. Build your plugin as a shared library
2. Copy the library file to VerseFinder's `plugins/` directory
3. Open VerseFinder and go to Tools → Plugin Manager
4. Your plugin should appear in the list
5. Click "Load Plugin" to activate it

## Debugging

### Enable Debug Mode

Set `debugMode = true` in your plugin configuration to enable detailed logging.

### Common Issues

- **Plugin not loading**: Check that all required exports are present
- **API access errors**: Ensure you're calling API methods only when plugin is active
- **Memory issues**: Check resource usage in Plugin Manager
- **Permission denied**: Grant necessary permissions through Plugin Manager UI

## Examples

See the `src/plugins/examples/` directory for complete example plugins:

- `enhanced_search_plugin.cpp`: Advanced search capabilities
- `simple_ui_plugin.cpp`: Basic UI extension

## Support

For questions and support:
- Check the VerseFinder documentation
- Review example plugins
- Submit issues on the VerseFinder GitHub repository

Happy plugin development!