#include "PlatformUtils.h"
#include <iostream>
#include <filesystem>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <climits>
#elif defined(__linux__)
#include <unistd.h>
#include <climits>
#include <pwd.h>
#elif defined(_WIN32)
#include <windows.h>
#include <shlobj.h>
#endif

PlatformUtils::PlatformUtils() {}
PlatformUtils::~PlatformUtils() {}

std::string PlatformUtils::getExecutablePath() {
#ifdef __APPLE__
    char buffer[PATH_MAX];
    uint32_t path_len = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &path_len) == 0) {
        return std::filesystem::path(buffer).parent_path().string();
    }
#elif defined(__linux__)
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return std::filesystem::path(buffer).parent_path().string();
    }
#elif defined(_WIN32)
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path().string();
#endif
    return "";
}

std::string PlatformUtils::getSettingsFilePath() {
    std::string settingsDir = getConfigDirectoryPath();
    
    // Fallback to executable directory if platform-specific path fails
    if (settingsDir.empty()) {
        settingsDir = getExecutablePath();
    }
    
    // Create directory if it doesn't exist
    try {
        std::filesystem::create_directories(settingsDir);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create settings directory: " << e.what() << std::endl;
        return getExecutablePath() + "/settings.json"; // Fallback
    }
    
    return settingsDir + "/settings.json";
}

std::string PlatformUtils::getConfigDirectoryPath() {
    std::string configDir;
    
#ifdef __APPLE__
    // Use ~/Library/Application Support/VerseFinder/ on macOS
    const char* home = getenv("HOME");
    if (home) {
        configDir = std::string(home) + "/Library/Application Support/VerseFinder";
    }
#elif defined(__linux__)
    // Use ~/.config/VerseFinder/ on Linux (XDG Base Directory)
    const char* configHome = getenv("XDG_CONFIG_HOME");
    if (configHome) {
        configDir = std::string(configHome) + "/VerseFinder";
    } else {
        const char* home = getenv("HOME");
        if (home) {
            configDir = std::string(home) + "/.config/VerseFinder";
        }
    }
#elif defined(_WIN32)
    // Use %APPDATA%/VerseFinder/ on Windows
    char appDataPath[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath) == S_OK) {
        configDir = std::string(appDataPath) + "\\VerseFinder";
    }
#endif
    
    return configDir;
}

bool PlatformUtils::isMacOS() {
#ifdef __APPLE__
    return true;
#else
    return false;
#endif
}

bool PlatformUtils::isLinux() {
#ifdef __linux__
    return true;
#else
    return false;
#endif
}

bool PlatformUtils::isWindows() {
#ifdef _WIN32
    return true;
#else
    return false;
#endif
}

std::string PlatformUtils::getHomeDirectory() {
#ifdef _WIN32
    const char* homeDir = getenv("USERPROFILE");
    if (!homeDir) {
        homeDir = getenv("HOMEDRIVE");
        if (homeDir) {
            const char* homePath = getenv("HOMEPATH");
            if (homePath) {
                return std::string(homeDir) + homePath;
            }
        }
    }
    return homeDir ? std::string(homeDir) : "";
#else
    const char* homeDir = getenv("HOME");
    return homeDir ? std::string(homeDir) : "";
#endif
}

std::string PlatformUtils::getAppDataDirectory() {
#ifdef _WIN32
    char appDataPath[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath) == S_OK) {
        return std::string(appDataPath);
    }
#endif
    return "";
}