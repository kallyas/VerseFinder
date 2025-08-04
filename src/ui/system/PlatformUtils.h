#ifndef PLATFORM_UTILS_H
#define PLATFORM_UTILS_H

#include <string>

class PlatformUtils {
public:
    PlatformUtils();
    ~PlatformUtils();

    // Platform-specific path utilities
    static std::string getExecutablePath();
    static std::string getSettingsFilePath();
    static std::string getConfigDirectoryPath();
    
    // Platform detection
    static bool isMacOS();
    static bool isLinux();
    static bool isWindows();

private:
    static std::string getHomeDirectory();
    static std::string getAppDataDirectory();
};

#endif // PLATFORM_UTILS_H