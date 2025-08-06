#ifndef PLUGIN_LOADER_H
#define PLUGIN_LOADER_H

#include "../interfaces/PluginInterfaces.h"
#include <string>
#include <memory>

#ifdef _WIN32
    #include <windows.h>
    typedef HMODULE LibraryHandle;
#else
    #include <dlfcn.h>
    typedef void* LibraryHandle;
#endif

namespace PluginSystem {

// Platform-specific library loading
class DynamicLibrary {
private:
    LibraryHandle handle;
    std::string library_path;
    std::string last_error;
    
public:
    DynamicLibrary() : handle(nullptr) {}
    ~DynamicLibrary() { unload(); }
    
    // Disable copy construction and assignment
    DynamicLibrary(const DynamicLibrary&) = delete;
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;
    
    // Enable move construction and assignment
    DynamicLibrary(DynamicLibrary&& other) noexcept 
        : handle(other.handle), library_path(std::move(other.library_path)) {
        other.handle = nullptr;
    }
    
    DynamicLibrary& operator=(DynamicLibrary&& other) noexcept {
        if (this != &other) {
            unload();
            handle = other.handle;
            library_path = std::move(other.library_path);
            other.handle = nullptr;
        }
        return *this;
    }
    
    bool load(const std::string& path) {
        if (handle) {
            unload();
        }
        
        library_path = path;
        
#ifdef _WIN32
        handle = LoadLibraryA(path.c_str());
        if (!handle) {
            DWORD error = GetLastError();
            last_error = "Failed to load library: " + path + " (Error: " + std::to_string(error) + ")";
            return false;
        }
#else
        // Clear any existing error
        dlerror();
        
        handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
        if (!handle) {
            const char* error = dlerror();
            last_error = "Failed to load library: " + path + " (" + (error ? error : "Unknown error") + ")";
            return false;
        }
#endif
        
        return true;
    }
    
    void unload() {
        if (handle) {
#ifdef _WIN32
            FreeLibrary(handle);
#else
            dlclose(handle);
#endif
            handle = nullptr;
        }
    }
    
    template<typename T>
    T getFunction(const std::string& functionName) {
        if (!handle) {
            last_error = "Library not loaded";
            return nullptr;
        }
        
#ifdef _WIN32
        FARPROC proc = GetProcAddress(handle, functionName.c_str());
        if (!proc) {
            last_error = "Function not found: " + functionName;
            return nullptr;
        }
        return reinterpret_cast<T>(proc);
#else
        // Clear any existing error
        dlerror();
        
        void* symbol = dlsym(handle, functionName.c_str());
        const char* error = dlerror();
        if (error) {
            last_error = "Function not found: " + functionName + " (" + error + ")";
            return nullptr;
        }
        return reinterpret_cast<T>(symbol);
#endif
    }
    
    bool isLoaded() const { return handle != nullptr; }
    const std::string& getPath() const { return library_path; }
    const std::string& getLastError() const { return last_error; }
};

// Plugin loader manages dynamic loading of plugin libraries
class PluginLoader {
private:
    std::unique_ptr<DynamicLibrary> library;
    std::unique_ptr<IPlugin> plugin_instance;
    CreatePluginFunc create_func;
    DestroyPluginFunc destroy_func;
    std::string plugin_type;
    std::string api_version;
    std::string last_error;
    
    bool validatePlugin() {
        if (!library || !library->isLoaded()) {
            last_error = "Library not loaded";
            return false;
        }
        
        // Get required functions
        create_func = library->getFunction<CreatePluginFunc>("createPlugin");
        if (!create_func) {
            last_error = "Missing createPlugin function";
            return false;
        }
        
        destroy_func = library->getFunction<DestroyPluginFunc>("destroyPlugin");
        if (!destroy_func) {
            last_error = "Missing destroyPlugin function";
            return false;
        }
        
        // Get API version
        auto getApiVersion = library->getFunction<const char*(*)()>("getPluginApiVersion");
        if (!getApiVersion) {
            last_error = "Missing getPluginApiVersion function";
            return false;
        }
        
        api_version = getApiVersion();
        if (api_version != "1.0") {
            last_error = "Unsupported API version: " + api_version;
            return false;
        }
        
        // Get plugin type
        auto getPluginType = library->getFunction<const char*(*)()>("getPluginType");
        if (!getPluginType) {
            last_error = "Missing getPluginType function";
            return false;
        }
        
        plugin_type = getPluginType();
        
        return true;
    }
    
public:
    PluginLoader() : library(std::make_unique<DynamicLibrary>()) {}
    
    ~PluginLoader() {
        unloadPlugin();
    }
    
    // Disable copy construction and assignment
    PluginLoader(const PluginLoader&) = delete;
    PluginLoader& operator=(const PluginLoader&) = delete;
    
    bool loadPlugin(const std::string& path) {
        unloadPlugin();
        
        if (!library->load(path)) {
            last_error = library->getLastError();
            return false;
        }
        
        if (!validatePlugin()) {
            library->unload();
            return false;
        }
        
        // Create plugin instance
        plugin_instance.reset(create_func());
        if (!plugin_instance) {
            last_error = "Failed to create plugin instance";
            library->unload();
            return false;
        }
        
        return true;
    }
    
    void unloadPlugin() {
        if (plugin_instance && destroy_func) {
            destroy_func(plugin_instance.release());
        }
        plugin_instance.reset();
        
        if (library) {
            library->unload();
        }
        
        create_func = nullptr;
        destroy_func = nullptr;
    }
    
    IPlugin* getPlugin() const {
        return plugin_instance.get();
    }
    
    const std::string& getPluginType() const {
        return plugin_type;
    }
    
    const std::string& getApiVersion() const {
        return api_version;
    }
    
    bool isLoaded() const {
        return library && library->isLoaded() && plugin_instance;
    }
    
    const std::string& getLastError() const {
        return last_error.empty() ? library->getLastError() : last_error;
    }
    
    std::string getLibraryPath() const {
        return library ? library->getPath() : "";
    }
};

} // namespace PluginSystem

#endif // PLUGIN_LOADER_H