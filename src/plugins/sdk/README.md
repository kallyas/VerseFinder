# Plugin SDK and Examples

This directory contains the plugin SDK for VerseFinder and example plugins demonstrating different plugin types.

## Plugin SDK Structure

### Headers
- `PluginInterfaces.h` - Core plugin interfaces
- `PluginAPI.h` - API for accessing VerseFinder functionality
- `PluginSecurity.h` - Security framework for plugins

### Templates
- `search_plugin_template/` - Template for search algorithm plugins
- `ui_plugin_template/` - Template for UI extension plugins
- `translation_plugin_template/` - Template for translation format plugins
- `theme_plugin_template/` - Template for theme engine plugins
- `integration_plugin_template/` - Template for integration connector plugins

### Examples
- `example_search_plugin/` - Example enhanced search plugin
- `example_ui_plugin/` - Example UI extension plugin
- `example_theme_plugin/` - Example theme plugin
- `example_script_plugin/` - Example Lua scripting plugin

## Building Plugins

Plugins are compiled as dynamic libraries (.dll on Windows, .so on Linux, .dylib on macOS).

### Required Exports

Every plugin must export these functions:
```cpp
extern "C" {
    PluginSystem::IPlugin* createPlugin();
    void destroyPlugin(PluginSystem::IPlugin* plugin);
    const char* getPluginApiVersion();
    const char* getPluginType();
}
```

### CMake Template

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyPlugin)

set(CMAKE_CXX_STANDARD 20)

# Include VerseFinder plugin headers
include_directories(${VERSEFINDER_ROOT}/src/plugins)

# Create plugin library
add_library(${PROJECT_NAME} SHARED
    plugin.cpp
    # Add other source files
)

# Link against system libraries as needed
target_link_libraries(${PROJECT_NAME}
    ${CMAKE_DL_LIBS}
)

# Set output name
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "lib${PROJECT_NAME}"
    PREFIX ""
    SUFFIX ".so"  # Or .dll/.dylib depending on platform
)
```

## Plugin Development Guidelines

1. **Always implement the base IPlugin interface**
2. **Handle errors gracefully and return meaningful error messages**
3. **Respect security permissions and resource limits**
4. **Use the PluginAPI to access VerseFinder functionality**
5. **Document your plugin's capabilities and requirements**
6. **Test your plugin thoroughly before distribution**
7. **Follow semantic versioning for plugin updates**

## Security Considerations

- Plugins run in a sandboxed environment by default
- Request only the permissions your plugin actually needs
- Never perform actions that could compromise user data
- Use signed plugins for distribution when possible
- Validate all user inputs and external data

## Plugin Distribution

Plugins can be distributed as:
- Single library files for installation
- Plugin packages with metadata and resources
- Through the official plugin marketplace (when available)
- Via GitHub releases or other version control systems