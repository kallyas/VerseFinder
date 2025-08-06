#include "PluginSecurity.h"
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <iostream>

namespace PluginSystem {

// SecurityContext implementation

void SecurityContext::grantPermission(const std::string& permission) {
    granted_permissions.insert(permission);
}

void SecurityContext::revokePermission(const std::string& permission) {
    granted_permissions.erase(permission);
}

bool SecurityContext::hasPermission(const std::string& permission) const {
    return is_trusted || granted_permissions.count(permission) > 0;
}

std::vector<std::string> SecurityContext::getGrantedPermissions() const {
    std::vector<std::string> result;
    for (const auto& perm : granted_permissions) {
        result.push_back(perm);
    }
    return result;
}

bool SecurityContext::checkMemoryUsage(size_t requestedMB) {
    return is_trusted || (current_memory_usage + requestedMB) <= limits.max_memory_mb;
}

bool SecurityContext::checkNetworkRequest() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - last_reset);
    
    if (elapsed.count() >= 1) {
        resetCounters();
    }
    
    return is_trusted || network_requests_count < limits.max_network_requests;
}

bool SecurityContext::checkDiskIO(size_t sizeMB) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - last_reset);
    
    if (elapsed.count() >= 1) {
        resetCounters();
    }
    
    return is_trusted || (disk_io_count + sizeMB) <= limits.max_disk_io_mb;
}

bool SecurityContext::checkCPUTime(size_t timeMs) {
    return is_trusted || timeMs <= limits.max_cpu_time_ms;
}

bool SecurityContext::checkFileAccess(const std::string& path, bool write) {
    if (is_trusted) return true;
    
    const auto& allowed_paths = write ? limits.allowed_write_paths : limits.allowed_read_paths;
    
    for (const auto& allowed : allowed_paths) {
        if (path.find(allowed) == 0) {
            return true;
        }
    }
    
    return false;
}

void SecurityContext::resetCounters() {
    network_requests_count = 0;
    disk_io_count = 0;
    last_reset = std::chrono::steady_clock::now();
}

// CodeSigner implementation (basic)

CodeSigner::SignatureInfo CodeSigner::verifySignature(const std::string& filePath) {
    SignatureInfo info;
    
    // Basic implementation - in real world, this would use platform-specific APIs
    // For now, just check if a .sig file exists
    std::string sigFile = filePath + ".sig";
    if (std::filesystem::exists(sigFile)) {
        info.is_signed = true;
        info.is_valid = true; // Simplified - would need actual verification
        info.signer = "Unknown";
    }
    
    return info;
}

bool CodeSigner::isTrustedSigner(const std::string& signer) {
    // Simplified implementation
    static std::unordered_set<std::string> trusted_signers = {
        "VerseFinder Official",
        "Microsoft Corporation",
        "Apple Inc."
    };
    
    return trusted_signers.count(signer) > 0;
}

void CodeSigner::addTrustedSigner(const std::string& signer) {
    // Would need persistent storage
}

void CodeSigner::removeTrustedSigner(const std::string& signer) {
    // Would need persistent storage
}

std::vector<std::string> CodeSigner::getTrustedSigners() {
    return {"VerseFinder Official", "Microsoft Corporation", "Apple Inc."};
}

// PluginSandbox implementation

bool PluginSandbox::allowFileRead(const std::string& path) {
    if (!sandbox_enabled || !context) return true;
    
    return context->hasPermission(Permissions::FILE_READ) && 
           context->checkFileAccess(path, false);
}

bool PluginSandbox::allowFileWrite(const std::string& path) {
    if (!sandbox_enabled || !context) return true;
    
    return context->hasPermission(Permissions::FILE_WRITE) && 
           context->checkFileAccess(path, true);
}

bool PluginSandbox::allowDirectoryAccess(const std::string& path) {
    if (!sandbox_enabled || !context) return true;
    
    return context->hasPermission(Permissions::FILE_READ) && 
           context->checkFileAccess(path, false);
}

bool PluginSandbox::allowNetworkAccess(const std::string& host, int port) {
    if (!sandbox_enabled || !context) return true;
    
    return context->hasPermission(Permissions::NETWORK_ACCESS) && 
           context->checkNetworkRequest();
}

bool PluginSandbox::allowHTTPRequest(const std::string& url) {
    if (!sandbox_enabled || !context) return true;
    
    return context->hasPermission(Permissions::NETWORK_ACCESS) && 
           context->checkNetworkRequest();
}

bool PluginSandbox::allowProcessExecution(const std::string& command) {
    if (!sandbox_enabled || !context) return true;
    
    return context->hasPermission(Permissions::PROCESS_EXECUTE);
}

bool PluginSandbox::allowLibraryLoading(const std::string& library) {
    if (!sandbox_enabled || !context) return true;
    
    return context->hasPermission(Permissions::LIBRARY_LOAD);
}

bool PluginSandbox::allowRegistryAccess(const std::string& key) {
    if (!sandbox_enabled || !context) return true;
    
    return context->hasPermission(Permissions::REGISTRY_ACCESS);
}

bool PluginSandbox::enforceMemoryLimit(size_t requestedMB) {
    if (!sandbox_enabled || !context) return true;
    
    return context->checkMemoryUsage(requestedMB);
}

bool PluginSandbox::enforceCPULimit(size_t timeMs) {
    if (!sandbox_enabled || !context) return true;
    
    return context->checkCPUTime(timeMs);
}

bool PluginSandbox::enforceDiskIOLimit(size_t sizeMB) {
    if (!sandbox_enabled || !context) return true;
    
    return context->checkDiskIO(sizeMB);
}

std::string PluginSandbox::normalizePath(const std::string& path) {
    try {
        return std::filesystem::canonical(path).string();
    } catch (...) {
        return path;
    }
}

bool PluginSandbox::isPathAllowed(const std::string& path, const std::vector<std::string>& allowedPaths) {
    std::string normalizedPath = normalizePath(path);
    
    for (const auto& allowed : allowedPaths) {
        std::string normalizedAllowed = normalizePath(allowed);
        if (normalizedPath.find(normalizedAllowed) == 0) {
            return true;
        }
    }
    
    return false;
}

// PluginSecurity implementation

PluginSecurity::PluginSecurity() {
    initializeDefaultPermissions();
}

PluginSecurity::~PluginSecurity() {
    shutdown();
}

bool PluginSecurity::initialize(const std::string& configPath) {
    security_config_path = configPath + "/security.conf";
    return loadSecurityConfig();
}

void PluginSecurity::shutdown() {
    saveSecurityConfig();
    contexts.clear();
}

void PluginSecurity::initializeDefaultPermissions() {
    available_permissions[Permissions::BIBLE_READ] = SecurityPermission(
        Permissions::BIBLE_READ, "Read Bible data", PermissionLevel::READ, false);
    
    available_permissions[Permissions::BIBLE_WRITE] = SecurityPermission(
        Permissions::BIBLE_WRITE, "Modify Bible data", PermissionLevel::WRITE, true);
    
    available_permissions[Permissions::FILE_READ] = SecurityPermission(
        Permissions::FILE_READ, "Read files", PermissionLevel::READ, false);
    
    available_permissions[Permissions::FILE_WRITE] = SecurityPermission(
        Permissions::FILE_WRITE, "Write files", PermissionLevel::WRITE, true);
    
    available_permissions[Permissions::NETWORK_ACCESS] = SecurityPermission(
        Permissions::NETWORK_ACCESS, "Access network", PermissionLevel::FULL, true);
    
    available_permissions[Permissions::UI_MODIFY] = SecurityPermission(
        Permissions::UI_MODIFY, "Modify user interface", PermissionLevel::WRITE, false);
    
    available_permissions[Permissions::SETTINGS_READ] = SecurityPermission(
        Permissions::SETTINGS_READ, "Read application settings", PermissionLevel::READ, false);
    
    available_permissions[Permissions::SETTINGS_WRITE] = SecurityPermission(
        Permissions::SETTINGS_WRITE, "Modify application settings", PermissionLevel::WRITE, true);
    
    available_permissions[Permissions::SYSTEM_INFO] = SecurityPermission(
        Permissions::SYSTEM_INFO, "Access system information", PermissionLevel::READ, false);
    
    available_permissions[Permissions::PROCESS_EXECUTE] = SecurityPermission(
        Permissions::PROCESS_EXECUTE, "Execute external processes", PermissionLevel::FULL, true);
    
    available_permissions[Permissions::LIBRARY_LOAD] = SecurityPermission(
        Permissions::LIBRARY_LOAD, "Load external libraries", PermissionLevel::FULL, true);
    
    available_permissions[Permissions::REGISTRY_ACCESS] = SecurityPermission(
        Permissions::REGISTRY_ACCESS, "Access system registry", PermissionLevel::FULL, true);
    
    available_permissions[Permissions::PRESENTATION_CONTROL] = SecurityPermission(
        Permissions::PRESENTATION_CONTROL, "Control presentation mode", PermissionLevel::WRITE, false);
    
    available_permissions[Permissions::PLUGIN_MANAGEMENT] = SecurityPermission(
        Permissions::PLUGIN_MANAGEMENT, "Manage other plugins", PermissionLevel::FULL, true);
}

bool PluginSecurity::loadSecurityConfig() {
    std::ifstream file(security_config_path);
    if (!file.is_open()) {
        return true; // No config file is fine, use defaults
    }
    
    // Simple config format: plugin_name.permission=granted
    std::string line;
    while (std::getline(file, line)) {
        size_t dotPos = line.find('.');
        size_t equalPos = line.find('=');
        
        if (dotPos != std::string::npos && equalPos != std::string::npos && dotPos < equalPos) {
            std::string pluginName = line.substr(0, dotPos);
            std::string permission = line.substr(dotPos + 1, equalPos - dotPos - 1);
            std::string value = line.substr(equalPos + 1);
            
            if (value == "granted") {
                if (contexts.find(pluginName) == contexts.end()) {
                    createContext(pluginName);
                }
                contexts[pluginName]->grantPermission(permission);
            }
        }
    }
    
    return true;
}

bool PluginSecurity::saveSecurityConfig() {
    std::ofstream file(security_config_path);
    if (!file.is_open()) {
        return false;
    }
    
    for (const auto& contextEntry : contexts) {
        const std::string& pluginName = contextEntry.first;
        const auto& context = contextEntry.second;
        
        for (const std::string& permission : context->getGrantedPermissions()) {
            file << pluginName << "." << permission << "=granted" << std::endl;
        }
    }
    
    return true;
}

SecurityContext* PluginSecurity::createContext(const std::string& pluginName) {
    contexts[pluginName] = std::make_unique<SecurityContext>(pluginName);
    return contexts[pluginName].get();
}

SecurityContext* PluginSecurity::getContext(const std::string& pluginName) {
    auto it = contexts.find(pluginName);
    return it != contexts.end() ? it->second.get() : nullptr;
}

void PluginSecurity::removeContext(const std::string& pluginName) {
    contexts.erase(pluginName);
}

std::vector<SecurityPermission> PluginSecurity::getAvailablePermissions() const {
    std::vector<SecurityPermission> result;
    for (const auto& entry : available_permissions) {
        result.push_back(entry.second);
    }
    return result;
}

bool PluginSecurity::grantPermission(const std::string& pluginName, const std::string& permission) {
    if (available_permissions.find(permission) == available_permissions.end()) {
        return false;
    }
    
    if (contexts.find(pluginName) == contexts.end()) {
        createContext(pluginName);
    }
    
    contexts[pluginName]->grantPermission(permission);
    return true;
}

bool PluginSecurity::revokePermission(const std::string& pluginName, const std::string& permission) {
    auto it = contexts.find(pluginName);
    if (it != contexts.end()) {
        it->second->revokePermission(permission);
        return true;
    }
    return false;
}

bool PluginSecurity::hasPermission(const std::string& pluginName, const std::string& permission) {
    auto it = contexts.find(pluginName);
    return it != contexts.end() && it->second->hasPermission(permission);
}

bool PluginSecurity::trustPlugin(const std::string& pluginName) {
    if (contexts.find(pluginName) == contexts.end()) {
        createContext(pluginName);
    }
    
    contexts[pluginName]->setTrusted(true);
    return true;
}

bool PluginSecurity::untrustPlugin(const std::string& pluginName) {
    auto it = contexts.find(pluginName);
    if (it != contexts.end()) {
        it->second->setTrusted(false);
        return true;
    }
    return false;
}

bool PluginSecurity::isPluginTrusted(const std::string& pluginName) {
    auto it = contexts.find(pluginName);
    return it != contexts.end() && it->second->isTrusted();
}

bool PluginSecurity::checkPluginPermissions(const std::string& pluginName, const PluginInfo& info) {
    // Basic safety checks - in a real implementation, this would be more sophisticated
    
    // Check if plugin requires dangerous permissions
    auto it = contexts.find(pluginName);
    if (it == contexts.end()) {
        // New plugin, create context with basic permissions
        createContext(pluginName);
        grantPermission(pluginName, Permissions::BIBLE_READ);
        grantPermission(pluginName, Permissions::UI_MODIFY);
    }
    
    return true; // Allow for now
}

bool PluginSecurity::validatePluginSafety(const std::string& filePath) {
    // Basic file validation
    if (!std::filesystem::exists(filePath)) {
        return false;
    }
    
    // Check file size (basic protection against huge files)
    try {
        auto fileSize = std::filesystem::file_size(filePath);
        if (fileSize > 100 * 1024 * 1024) { // 100MB limit
            return false;
        }
    } catch (...) {
        return false;
    }
    
    // If code signing is required, verify signature
    if (code_signing_required) {
        return verifyPluginSignature(filePath);
    }
    
    return true;
}

bool PluginSecurity::scanForMalware(const std::string& filePath) {
    // Basic malware scanning - in real implementation, integrate with antivirus
    // For now, just check against known bad patterns
    
    std::string filename = std::filesystem::path(filePath).filename().string();
    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
    
    // Simple blacklist
    std::vector<std::string> suspicious_names = {
        "keylogger", "trojan", "virus", "malware", "backdoor"
    };
    
    for (const auto& suspicious : suspicious_names) {
        if (filename.find(suspicious) != std::string::npos) {
            return false;
        }
    }
    
    return true;
}

PluginSandbox PluginSecurity::createSandbox(const std::string& pluginName) {
    SecurityContext* context = getContext(pluginName);
    if (!context) {
        context = createContext(pluginName);
    }
    
    return PluginSandbox(context);
}

bool PluginSecurity::verifyPluginSignature(const std::string& filePath) {
    auto sigInfo = CodeSigner::verifySignature(filePath);
    return sigInfo.is_signed && sigInfo.is_valid && CodeSigner::isTrustedSigner(sigInfo.signer);
}

void PluginSecurity::recordSecurityViolation(const std::string& pluginName, const std::string& violation) {
    security_violations[pluginName].push_back(violation);
    
    // Log to console for now
    std::cerr << "Security violation in plugin " << pluginName << ": " << violation << std::endl;
}

std::vector<std::string> PluginSecurity::getSecurityViolations(const std::string& pluginName) {
    auto it = security_violations.find(pluginName);
    return it != security_violations.end() ? it->second : std::vector<std::string>();
}

void PluginSecurity::clearSecurityViolations(const std::string& pluginName) {
    security_violations.erase(pluginName);
}

void PluginSecurity::updateResourceUsage() {
    // Update resource counters for all contexts
    for (auto& contextEntry : contexts) {
        // In a real implementation, this would query system for actual resource usage
        // For now, just reset periodic counters
        contextEntry.second->resetCounters();
    }
}

} // namespace PluginSystem