#include "../manager/PluginManager.h"
#include "../examples/enhanced_search_plugin.cpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "=== VerseFinder Plugin System Test ===" << std::endl;
    
    // Test 1: Plugin interfaces and factories
    std::cout << "Test 1: Plugin creation and destruction..." << std::endl;
    {
        auto plugin = createPlugin();
        assert(plugin != nullptr);
        assert(plugin->getState() == PluginSystem::PluginState::UNLOADED);
        
        // Test plugin info
        auto info = plugin->getInfo();
        assert(!info.name.empty());
        assert(!info.description.empty());
        assert(!info.author.empty());
        
        destroyPlugin(plugin);
        std::cout << "✓ Plugin creation and destruction works" << std::endl;
    }
    
    // Test 2: Plugin API version
    std::cout << "Test 2: Plugin API version..." << std::endl;
    {
        auto version = getPluginApiVersion();
        assert(std::string(version) == "1.0");
        std::cout << "✓ Plugin API version: " << version << std::endl;
    }
    
    // Test 3: Plugin type
    std::cout << "Test 3: Plugin type..." << std::endl;
    {
        auto type = getPluginType();
        assert(std::string(type) == "search");
        std::cout << "✓ Plugin type: " << type << std::endl;
    }
    
    // Test 4: Plugin initialization and lifecycle
    std::cout << "Test 4: Plugin lifecycle..." << std::endl;
    {
        auto plugin = createPlugin();
        
        // Test initialization
        bool initialized = plugin->initialize();
        assert(initialized);
        assert(plugin->getState() == PluginSystem::PluginState::LOADED);
        
        // Test activation
        plugin->onActivate();
        assert(plugin->getState() == PluginSystem::PluginState::ACTIVE);
        
        // Test deactivation
        plugin->onDeactivate();
        assert(plugin->getState() == PluginSystem::PluginState::LOADED);
        
        // Test shutdown
        plugin->shutdown();
        assert(plugin->getState() == PluginSystem::PluginState::UNLOADED);
        
        destroyPlugin(plugin);
        std::cout << "✓ Plugin lifecycle works correctly" << std::endl;
    }
    
    // Test 5: Plugin configuration
    std::cout << "Test 5: Plugin configuration..." << std::endl;
    {
        auto plugin = createPlugin();
        plugin->initialize();
        
        PluginSystem::PluginConfig config;
        config.set("test_setting", "test_value");
        config.set("test_number", "42");
        config.set("test_bool", "true");
        
        bool configured = plugin->configure(config);
        assert(configured);
        
        plugin->shutdown();
        destroyPlugin(plugin);
        std::cout << "✓ Plugin configuration works" << std::endl;
    }
    
    // Test 6: Search plugin specific functionality
    std::cout << "Test 6: Search plugin functionality..." << std::endl;
    {
        auto plugin = dynamic_cast<PluginSystem::ISearchPlugin*>(createPlugin());
        assert(plugin != nullptr);
        
        plugin->initialize();
        plugin->onActivate();
        
        // Test search capabilities
        assert(plugin->supportsTranslation("KJV"));
        assert(plugin->getSearchQuality("test query") > 0.0);
        
        auto options = plugin->getSupportedOptions();
        assert(!options.empty());
        
        auto description = plugin->getSearchDescription();
        assert(!description.empty());
        
        plugin->shutdown();
        destroyPlugin(plugin);
        std::cout << "✓ Search plugin functionality works" << std::endl;
    }
    
    std::cout << std::endl << "=== All Plugin Tests Passed! ===" << std::endl;
    std::cout << "Plugin system is working correctly." << std::endl;
    
    return 0;
}