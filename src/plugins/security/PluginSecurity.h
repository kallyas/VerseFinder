#ifndef PLUGIN_SECURITY_H
#define PLUGIN_SECURITY_H

#include "../interfaces/PluginInterfaces.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <chrono>

namespace PluginSystem {

// Security permission levels
enum class PermissionLevel {
    NONE = 0,
    READ = 1,
    WRITE = 2,
    FULL = 3
};

// Security permissions
struct SecurityPermission {
    std::string name;
    std::string description;
    PermissionLevel level;
    bool dangerous;
    
    SecurityPermission() : level(PermissionLevel::NONE), dangerous(false) {}
    
    SecurityPermission(const std::string& n, const std::string& desc, 
                      PermissionLevel lvl, bool isDangerous = false)
        : name(n), description(desc), level(lvl), dangerous(isDangerous) {}
};

// Resource usage limits
struct ResourceLimits {
    size_t max_memory_mb = 100;        // Maximum memory usage in MB
    size_t max_file_size_mb = 10;      // Maximum file size to read/write in MB
    size_t max_network_requests = 100; // Maximum network requests per minute
    size_t max_cpu_time_ms = 1000;     // Maximum CPU time per operation in ms
    size_t max_disk_io_mb = 50;        // Maximum disk I/O per minute in MB
    
    // File system access
    std::vector<std::string> allowed_read_paths;
    std::vector<std::string> allowed_write_paths;
    bool allow_network_access = false;
    bool allow_subprocess = false;
    bool allow_dll_loading = false;
};

// Security context for plugin execution
class SecurityContext {
private:
    std::string plugin_name;
    std::unordered_set<std::string> granted_permissions;
    ResourceLimits limits;
    bool is_trusted;
    
    // Resource tracking
    size_t current_memory_usage = 0;
    size_t network_requests_count = 0;
    size_t disk_io_count = 0;
    std::chrono::steady_clock::time_point last_reset;
    
public:
    explicit SecurityContext(const std::string& name) 
        : plugin_name(name), is_trusted(false), 
          last_reset(std::chrono::steady_clock::now()) {}
    
    // Permission management
    void grantPermission(const std::string& permission);
    void revokePermission(const std::string& permission);
    bool hasPermission(const std::string& permission) const;
    std::vector<std::string> getGrantedPermissions() const;
    
    // Trust management
    void setTrusted(bool trusted) { is_trusted = trusted; }
    bool isTrusted() const { return is_trusted; }
    
    // Resource limits
    void setResourceLimits(const ResourceLimits& newLimits) { limits = newLimits; }
    const ResourceLimits& getResourceLimits() const { return limits; }
    
    // Resource tracking
    bool checkMemoryUsage(size_t requestedMB);
    bool checkNetworkRequest();
    bool checkDiskIO(size_t sizeMB);
    bool checkCPUTime(size_t timeMs);
    bool checkFileAccess(const std::string& path, bool write);
    
    void updateMemoryUsage(size_t currentMB) { current_memory_usage = currentMB; }
    void recordNetworkRequest() { network_requests_count++; }
    void recordDiskIO(size_t sizeMB) { disk_io_count += sizeMB; }
    
    // Reset counters (called periodically)
    void resetCounters();
    
    const std::string& getPluginName() const { return plugin_name; }
};

// Code signing verification
class CodeSigner {
public:
    struct SignatureInfo {
        bool is_signed = false;
        bool is_valid = false;
        std::string signer;
        std::string certificate_chain;
        std::chrono::system_clock::time_point expiry_date;
        std::string hash;
    };
    
    // Verify code signature
    static SignatureInfo verifySignature(const std::string& filePath);
    
    // Check if signature is trusted
    static bool isTrustedSigner(const std::string& signer);
    
    // Add trusted signer
    static void addTrustedSigner(const std::string& signer);
    
    // Remove trusted signer
    static void removeTrustedSigner(const std::string& signer);
    
    // Get all trusted signers
    static std::vector<std::string> getTrustedSigners();
};

// Sandbox environment for plugin execution
class PluginSandbox {
private:
    SecurityContext* context;
    bool sandbox_enabled;
    
public:
    explicit PluginSandbox(SecurityContext* ctx) 
        : context(ctx), sandbox_enabled(true) {}
    
    // Enable/disable sandbox
    void enableSandbox(bool enable) { sandbox_enabled = enable; }
    bool isSandboxEnabled() const { return sandbox_enabled; }
    
    // File system access control
    bool allowFileRead(const std::string& path);
    bool allowFileWrite(const std::string& path);
    bool allowDirectoryAccess(const std::string& path);
    
    // Network access control
    bool allowNetworkAccess(const std::string& host, int port);
    bool allowHTTPRequest(const std::string& url);
    
    // System access control
    bool allowProcessExecution(const std::string& command);
    bool allowLibraryLoading(const std::string& library);
    bool allowRegistryAccess(const std::string& key);
    
    // Resource monitoring
    bool enforceMemoryLimit(size_t requestedMB);
    bool enforceCPULimit(size_t timeMs);
    bool enforceDiskIOLimit(size_t sizeMB);
    
private:
    bool isPathAllowed(const std::string& path, const std::vector<std::string>& allowedPaths);
    std::string normalizePath(const std::string& path);
};

// Main security manager
class PluginSecurity {
private:
    std::unordered_map<std::string, std::unique_ptr<SecurityContext>> contexts;
    std::unordered_map<std::string, SecurityPermission> available_permissions;
    std::string security_config_path;
    bool global_sandbox_enabled = true;
    
    void initializeDefaultPermissions();
    bool loadSecurityConfig();
    bool saveSecurityConfig();
    
public:
    PluginSecurity();
    ~PluginSecurity();
    
    // Initialization
    bool initialize(const std::string& configPath);
    void shutdown();
    
    // Plugin security context management
    SecurityContext* createContext(const std::string& pluginName);
    SecurityContext* getContext(const std::string& pluginName);
    void removeContext(const std::string& pluginName);
    
    // Permission management
    std::vector<SecurityPermission> getAvailablePermissions() const;
    bool grantPermission(const std::string& pluginName, const std::string& permission);
    bool revokePermission(const std::string& pluginName, const std::string& permission);
    bool hasPermission(const std::string& pluginName, const std::string& permission);
    
    // Trust management
    bool trustPlugin(const std::string& pluginName);
    bool untrustPlugin(const std::string& pluginName);
    bool isPluginTrusted(const std::string& pluginName);
    
    // Resource limits
    bool setResourceLimits(const std::string& pluginName, const ResourceLimits& limits);
    ResourceLimits getResourceLimits(const std::string& pluginName);
    
    // Plugin validation
    bool checkPluginPermissions(const std::string& pluginName, const PluginInfo& info);
    bool validatePluginSafety(const std::string& filePath);
    bool scanForMalware(const std::string& filePath);
    
    // Sandbox management
    void enableGlobalSandbox(bool enable) { global_sandbox_enabled = enable; }
    bool isGlobalSandboxEnabled() const { return global_sandbox_enabled; }
    
    PluginSandbox createSandbox(const std::string& pluginName);
    
    // Code signing
    bool requireCodeSigning(bool require);
    bool isCodeSigningRequired() const;
    bool verifyPluginSignature(const std::string& filePath);
    
    // Security monitoring
    void recordSecurityViolation(const std::string& pluginName, const std::string& violation);
    std::vector<std::string> getSecurityViolations(const std::string& pluginName);
    void clearSecurityViolations(const std::string& pluginName);
    
    // Update security contexts
    void updateResourceUsage();
    
private:
    bool code_signing_required = false;
    std::unordered_map<std::string, std::vector<std::string>> security_violations;
    
    bool isSystemPath(const std::string& path);
    bool isSafePath(const std::string& path);
    std::string sanitizePath(const std::string& path);
};

// Predefined security permissions
namespace Permissions {
    constexpr const char* BIBLE_READ = "bible.read";
    constexpr const char* BIBLE_WRITE = "bible.write";
    constexpr const char* FILE_READ = "file.read";
    constexpr const char* FILE_WRITE = "file.write";
    constexpr const char* NETWORK_ACCESS = "network.access";
    constexpr const char* UI_MODIFY = "ui.modify";
    constexpr const char* SETTINGS_READ = "settings.read";
    constexpr const char* SETTINGS_WRITE = "settings.write";
    constexpr const char* SYSTEM_INFO = "system.info";
    constexpr const char* PROCESS_EXECUTE = "process.execute";
    constexpr const char* LIBRARY_LOAD = "library.load";
    constexpr const char* REGISTRY_ACCESS = "registry.access";
    constexpr const char* PRESENTATION_CONTROL = "presentation.control";
    constexpr const char* PLUGIN_MANAGEMENT = "plugin.management";
}

} // namespace PluginSystem

#endif // PLUGIN_SECURITY_H