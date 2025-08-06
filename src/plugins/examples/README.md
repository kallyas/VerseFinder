# VerseFinder Example Plugins

This directory contains example plugins that showcase the VerseFinder plugin system capabilities. These plugins demonstrate different plugin interfaces and real-world use cases.

## Available Example Plugins

### 1. Enhanced Search Plugin
- **Type**: Search Algorithm Plugin (`ISearchPlugin`)
- **Features**: Advanced search with regex, wildcards, and semantic capabilities
- **Use Case**: Demonstrates how to extend VerseFinder's search functionality
- **File**: `enhanced_search_plugin.cpp`

### 2. Simple UI Plugin
- **Type**: UI Extension Plugin (`IUIPlugin`)
- **Features**: Adds custom menu items and UI panels
- **Use Case**: Shows how to extend the VerseFinder interface
- **File**: `simple_ui_plugin.cpp`

### 3. PDF Export Plugin ⭐ **NEW**
- **Type**: Export Format Plugin (`IExportPlugin`)
- **Features**: Export verses and service plans to formatted PDF-style documents
- **Use Case**: Demonstrates file export capabilities with custom formatting
- **File**: `pdf_export_plugin.cpp`

## PDF Export Plugin Features

The PDF Export Plugin is a comprehensive example that showcases:

### Core Functionality
- Export single verses to formatted documents
- Export multiple verses with customizable layout
- Export service plans with professional formatting
- HTML output with CSS styling (easily convertible to PDF)

### Customization Options
- **Font Settings**: Font family and size configuration
- **Page Layout**: Page size and margin settings  
- **Headers/Footers**: Configurable header text and page numbering
- **Styling**: Verse separation, background colors, and borders
- **Content**: Custom titles and export metadata

### Configuration Example
```cpp
PluginConfig config;
config.set("fontSize", "14");
config.set("fontFamily", "Georgia");
config.set("includeHeader", "true");
config.set("headerText", "Sunday Service Verses");
config.set("titleText", "Morning Worship");
config.set("separateVerses", "true");
```

### Usage Examples

#### Export Single Verse
```cpp
std::string verse = "For God so loved the world...";
std::string reference = "John 3:16";
plugin->exportVerse(verse, reference, "john3_16.pdf");
```

#### Export Multiple Verses
```cpp
std::vector<std::string> verses = {
    "In the beginning was the Word...",
    "For God so loved the world...",
    "I can do all things through Christ..."
};
std::vector<std::string> references = {
    "John 1:1", "John 3:16", "Philippians 4:13"
};
plugin->exportVerses(verses, references, "service_verses.pdf");
```

#### Export Service Plan
```cpp
std::string plan = "Opening Prayer\nHymn: Amazing Grace\nScripture Reading: Psalm 23\nSermon: Faith in Action\nClosing Prayer";
plugin->exportServicePlan(plan, "service_plan.pdf");
```

## Building the Example Plugins

### Prerequisites
- CMake 3.16 or higher
- C++20 compatible compiler
- VerseFinder core libraries

### Build Commands
```bash
# Navigate to the examples directory
cd src/plugins/examples

# Generate build files
cmake .

# Build all example plugins
make

# Or build specific plugin
make pdf_export_plugin
```

### Output
Built plugins are automatically copied to the `plugins/` directory in the VerseFinder root and can be loaded through the Plugin Manager.

## Plugin Development Guide

These examples demonstrate key plugin development concepts:

### 1. Plugin Interface Implementation
Each plugin implements one of the core interfaces:
- `IPlugin` (base interface - required)
- `ISearchPlugin`, `IUIPlugin`, `IExportPlugin`, etc. (specialized interfaces)

### 2. Lifecycle Management
```cpp
bool initialize() override;      // Plugin startup
void shutdown() override;        // Plugin cleanup  
void onActivate() override;      // When enabled
void onDeactivate() override;    // When disabled
```

### 3. Configuration Handling
```cpp
bool configure(const PluginConfig& config) override {
    // Read settings from config
    fontSize = config.getString("fontSize", "12");
    return true;
}
```

### 4. Error Handling
```cpp
try {
    // Plugin operation
    return true;
} catch (const std::exception& e) {
    last_error = "Operation failed: " + std::string(e.what());
    return false;
}
```

### 5. Required Export Functions
```cpp
extern "C" {
    PluginSystem::IPlugin* createPlugin();
    void destroyPlugin(PluginSystem::IPlugin* plugin);
    const char* getPluginApiVersion();
    const char* getPluginType();
}
```

## Testing the Plugins

1. **Build the plugins** using the commands above
2. **Start VerseFinder** application
3. **Open Plugin Manager** via Tools → Plugin Manager (Ctrl+Shift+P)
4. **Load plugins** from the plugins directory
5. **Configure and activate** the desired plugins
6. **Test functionality** through the VerseFinder interface

## Plugin Security

These example plugins demonstrate security best practices:
- Input validation and sanitization
- Error handling and graceful degradation
- Resource management and cleanup
- Safe file operations

## Extending the Examples

You can modify these plugins to add new features:
- **PDF Export**: Add real PDF generation using libraries like libharu
- **Enhanced Search**: Implement AI-powered search algorithms
- **UI Plugin**: Create custom panels with advanced controls

## Support and Documentation

For more information about plugin development:
- See the Plugin SDK documentation in `src/plugins/sdk/`
- Review the Plugin Development Guide
- Check the API reference in `src/plugins/api/`
- Browse additional templates in `src/plugins/sdk/templates/`