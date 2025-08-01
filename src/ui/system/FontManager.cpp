#include "FontManager.h"
#include <iostream>
#include <filesystem>
#include <vector>

#ifdef __APPLE__
    #include <mach-o/dyld.h>
#elif defined(_WIN32)
    #include <windows.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <limits.h>
#endif

FontManager::FontManager() 
    : regular_font(nullptr)
    , system_font_size(16.0f) {
}

FontManager::~FontManager() {
    // ImGui manages font cleanup
}

std::string FontManager::getExecutablePath() const {
    std::string path;
    
#ifdef __APPLE__
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    path.resize(size);
    _NSGetExecutablePath(&path[0], &size);
    path.resize(size - 1); // Remove null terminator
#elif defined(_WIN32)
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    path = buffer;
#elif defined(__linux__)
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        path = buffer;
    }
#endif

    // Get directory path
    size_t last_slash = path.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        path = path.substr(0, last_slash);
    }
    
    return path;
}

float FontManager::getSystemFontSize() const {
    return system_font_size;
}

bool FontManager::initializeFonts() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Detect system font size
#ifdef __APPLE__
    system_font_size = 14.0f; // Standard macOS font size
#elif defined(_WIN32)
    system_font_size = 16.0f; // Standard Windows font size
#else
    system_font_size = 14.0f; // Standard Linux font size
#endif

    // Load regular font first
    if (!loadRegularFont()) {
        std::cerr << "Warning: Failed to load regular font, using default" << std::endl;
        regular_font = io.Fonts->AddFontDefault();
    }


    // Build font atlas
    io.Fonts->Build();

    return true;
}

bool FontManager::loadRegularFont() {
    ImGuiIO& io = ImGui::GetIO();
    
    // List of font paths to try in order of preference
    std::vector<std::string> font_paths;
    
    // Try compile-time path first
    #ifdef GENTIUM_FONT_PATH
        font_paths.push_back(GENTIUM_FONT_PATH);
    #endif
    
    // Runtime path resolution
    std::string exe_dir = getExecutablePath();
    font_paths.push_back(exe_dir + "/fonts/Gentium_Plus/GentiumPlus-Regular.ttf");
    font_paths.push_back(exe_dir + "/fonts/arial/ARIAL.TTF");
    font_paths.push_back(exe_dir + "/fonts/arial/arial.ttf");
    
    // System fonts as fallback
    #ifdef __APPLE__
        font_paths.push_back("/System/Library/Fonts/Helvetica.ttc");
        font_paths.push_back("/System/Library/Fonts/Arial.ttf");
        font_paths.push_back("/System/Library/Fonts/LucidaGrande.ttc");
    #elif defined(_WIN32)
        font_paths.push_back("C:/Windows/Fonts/arial.ttf");
        font_paths.push_back("C:/Windows/Fonts/segoeui.ttf");
        font_paths.push_back("C:/Windows/Fonts/tahoma.ttf");
    #else
        font_paths.push_back("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
        font_paths.push_back("/usr/share/fonts/TTF/DejaVuSans.ttf");
        font_paths.push_back("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf");
    #endif
    
    // Try each font path
    for (const auto& path : font_paths) {
        if (std::filesystem::exists(path)) {
            std::cout << "Trying to load font: " << path << std::endl;
            regular_font = io.Fonts->AddFontFromFileTTF(path.c_str(), system_font_size);
            if (regular_font) {
                std::cout << "Successfully loaded font: " << path << std::endl;
                return true;
            } else {
                std::cerr << "Failed to load font: " << path << std::endl;
            }
        } else {
            std::cout << "Font file does not exist: " << path << std::endl;
        }
    }
    
    std::cerr << "Warning: Could not load any custom fonts, will use default ImGui font" << std::endl;
    return false;
}

