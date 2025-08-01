#include "VerseFinderApp.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <regex>
#include <ctime>
#include <cstdlib>
#include <thread>
#include <future>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <objc/objc-runtime.h>
#include <CoreText/CoreText.h>
#include <climits>
#elif defined(__linux__)
#include <unistd.h>
#include <climits>
#include <pwd.h>
#elif defined(_WIN32)
#include <windows.h>
#include <shlobj.h>
#endif

VerseFinderApp::VerseFinderApp() : window(nullptr), presentation_window(nullptr) {}

VerseFinderApp::~VerseFinderApp() {
    cleanup();
}

float VerseFinderApp::getSystemFontSize() const {
#ifdef __APPLE__
    // Get the system font size using Core Text on macOS
    CTFontRef systemFont = CTFontCreateUIFontForLanguage(kCTFontUIFontSystem, 0.0, NULL);
    if (systemFont) {
        CGFloat fontSize = 24.0f; // Default size
        CFRelease(systemFont);
        // Return the system font size, but ensure it's at least 12 and at most 24 for readability
        return std::max(12.0f, std::min(24.0f, static_cast<float>(fontSize)));
    }
#endif
    // Fallback to a reasonable default size
    return 16.0f;
}

void VerseFinderApp::glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

std::string VerseFinderApp::getExecutablePath() const {
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

std::string VerseFinderApp::getSettingsFilePath() const {
    std::string settingsDir;
    
#ifdef __APPLE__
    // Use ~/Library/Application Support/VerseFinder/ on macOS
    const char* home = getenv("HOME");
    if (home) {
        settingsDir = std::string(home) + "/Library/Application Support/VerseFinder";
    }
#elif defined(__linux__)
    // Use ~/.config/VerseFinder/ on Linux (XDG Base Directory)
    const char* configHome = getenv("XDG_CONFIG_HOME");
    if (configHome) {
        settingsDir = std::string(configHome) + "/VerseFinder";
    } else {
        const char* home = getenv("HOME");
        if (home) {
            settingsDir = std::string(home) + "/.config/VerseFinder";
        }
    }
#elif defined(_WIN32)
    // Use %APPDATA%/VerseFinder/ on Windows
    char appDataPath[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath) == S_OK) {
        settingsDir = std::string(appDataPath) + "\\VerseFinder";
    }
#endif
    
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

bool VerseFinderApp::init() {
    // Load settings first
    loadSettings();
    
    // Setup error callback
    glfwSetErrorCallback(glfwErrorCallback);
    
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // GL 3.2+ Core Profile required for macOS, GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#endif
    
    // Create window with saved size or defaults
    int windowWidth = userSettings.display.windowWidth;
    int windowHeight = userSettings.display.windowHeight;
    window = glfwCreateWindow(windowWidth, windowHeight, "VerseFinder - Bible Search for Churches", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    // Set window position if saved and valid
    if (userSettings.display.rememberWindowState && 
        userSettings.display.windowPosX >= 0 && userSettings.display.windowPosY >= 0) {
        glfwSetWindowPos(window, userSettings.display.windowPosX, userSettings.display.windowPosY);
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    
    // Initialize OpenGL loader
#ifdef IMGUI_IMPL_OPENGL_LOADER_GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return false;
    }
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    if (gl3wInit() != 0) {
        std::cerr << "Failed to initialize GL3W" << std::endl;
        return false;
    }
#elif defined(IMGUI_IMPL_OPENGL_LOADER_CUSTOM)
    // Custom loader - no initialization needed on macOS
    std::cout << "Using system OpenGL (no loader initialization required)" << std::endl;
#endif
    
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Setup style
    setupImGuiStyle();
    
    // Apply additional styling
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    // Load fonts with symbol support using system font size
    float systemFontSize = getSystemFontSize();
    
    // Try to load a custom font with fallback to default
    std::vector<std::string> font_paths;
    
    // Use compile-time defined font path from CMake
    #ifdef GENTIUM_FONT_PATH
        font_paths.push_back(GENTIUM_FONT_PATH);
    #endif
    
    // Runtime path resolution
    std::string exe_dir = getExecutablePath();
    font_paths.push_back(exe_dir + "/fonts/Gentium_Plus/GentiumPlus-Regular.ttf");
    font_paths.push_back(exe_dir + "/fonts/arial/ARIAL.TTF");
    
    // Try to load fonts in order of preference
    ImFont* loaded_font = nullptr;
    for (const auto& path : font_paths) {
        if (std::filesystem::exists(path)) {
            loaded_font = io.Fonts->AddFontFromFileTTF(path.c_str(), systemFontSize);
            if (loaded_font) {
                std::cout << "Loaded font: " << path << std::endl;
                break;
            }
        }
    }
    
    // If no custom font loaded, use default
    if (!loaded_font) {
        std::cout << "Using default ImGui font" << std::endl;
        io.Fonts->AddFontDefault();
    }
    
    // Add Unicode ranges for symbol/emoji support (limited to 16-bit ranges)
    static const ImWchar ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x206F, // General Punctuation
        0x2190, 0x21FF, // Arrows
        0x2600, 0x26FF, // Miscellaneous Symbols (includes some emoji-like symbols)
        0x2700, 0x27BF, // Dingbats
        0x3000, 0x30FF, // CJK Symbols and Punctuation, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
        0,
    };
    
    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphMinAdvanceX = systemFontSize; // Ensure symbols are properly spaced
    
    // Try to add a system font with better symbol support
    #ifdef __APPLE__
        // Try system UI font which has good symbol support
        io.Fonts->AddFontFromFileTTF("/System/Library/Fonts/Helvetica.ttc", systemFontSize, &config, ranges);
    #elif _WIN32
        io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", systemFontSize, &config, ranges);
    #else
        // On Linux, try DejaVu Sans which has good symbol coverage
        io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", systemFontSize, &config, ranges);
    #endif
    
    // Setup translations directory and load all translations
    std::string translations_path = getExecutablePath() + "/translations";
    bible.setTranslationsDirectory(translations_path);
    bible.loadAllTranslations();
    
    // Scan for existing translation files and update status
    scanForExistingTranslations();
    updateAvailableTranslationStatus();
    
    // Apply loaded settings to application state
    fuzzy_search_enabled = userSettings.search.fuzzySearchEnabled;
    bible.enableFuzzySearch(fuzzy_search_enabled);
    auto_search = userSettings.search.autoSearch;
    show_performance_stats = userSettings.search.showPerformanceStats;
    
    return true;
}

void VerseFinderApp::setupImGuiStyle() {
    // Apply theme based on user settings
    if (userSettings.display.colorTheme == "light") {
        applyLightTheme();
    } else if (userSettings.display.colorTheme == "blue") {
        applyBlueTheme();
    } else if (userSettings.display.colorTheme == "green") {
        applyGreenTheme();
    } else {
        applyDarkTheme(); // Default to dark theme
    }
    
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Borders
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;
    
    // Rounding
    style.WindowRounding = 8.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;
    
    // Spacing
    style.WindowPadding = ImVec2(12, 12);
    style.FramePadding = ImVec2(8, 6);
    style.ItemSpacing = ImVec2(8, 6);
    style.ItemInnerSpacing = ImVec2(6, 4);
    style.IndentSpacing = 20.0f;
    style.ScrollbarSize = 16.0f;
    style.GrabMinSize = 12.0f;
    
    // Apply font scaling
    ImGui::GetIO().FontGlobalScale = userSettings.display.fontSize / 16.0f;
}

void VerseFinderApp::applyDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    // Dark theme colors
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.15f, 0.16f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.28f, 0.28f, 0.29f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.32f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.29f, 0.62f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.35f, 0.58f, 0.86f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    // Docking colors removed - not available in this ImGui version
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

void VerseFinderApp::applyLightTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    // Light theme colors
    colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.46f, 0.54f, 0.80f, 0.60f);
    colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.35f, 0.35f, 0.35f, 0.17f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.76f, 0.80f, 0.84f, 0.93f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.60f, 0.73f, 0.88f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.92f, 0.93f, 0.94f, 0.99f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.74f, 0.82f, 0.91f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.78f, 0.87f, 0.98f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.57f, 0.57f, 0.64f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.68f, 0.68f, 0.74f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.30f, 0.30f, 0.30f, 0.09f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

void VerseFinderApp::applyBlueTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    // Blue theme colors (dark blue base)
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.95f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.60f, 0.70f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.08f, 0.16f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.12f, 0.20f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.04f, 0.06f, 0.12f, 0.95f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.30f, 0.50f, 0.60f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.15f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.25f, 0.40f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.35f, 0.55f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.06f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.08f, 0.12f, 0.20f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.04f, 0.08f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.35f, 0.55f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.45f, 0.65f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.55f, 0.75f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.40f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.60f, 0.95f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.45f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.40f, 0.80f, 0.60f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.50f, 0.90f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.35f, 0.75f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.40f, 0.80f, 0.45f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.50f, 0.90f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.45f, 0.85f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.30f, 0.50f, 0.60f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.30f, 0.50f, 0.80f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.35f, 0.55f, 0.85f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.20f, 0.40f, 0.80f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.30f, 0.50f, 0.90f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.35f, 0.55f, 0.95f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.25f, 0.50f, 0.86f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.30f, 0.50f, 0.90f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.35f, 0.70f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.05f, 0.08f, 0.15f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.12f, 0.20f, 0.40f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.12f, 0.20f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.25f, 0.35f, 0.55f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.18f, 0.28f, 0.45f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.30f, 0.50f, 0.90f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.30f, 0.50f, 0.90f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

void VerseFinderApp::applyGreenTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    // Green theme colors (dark green base)
    colors[ImGuiCol_Text] = ImVec4(0.90f, 1.00f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.70f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.16f, 0.08f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.20f, 0.12f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.06f, 0.12f, 0.06f, 0.95f);
    colors[ImGuiCol_Border] = ImVec4(0.30f, 0.50f, 0.30f, 0.60f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.25f, 0.15f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.40f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.55f, 0.35f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.12f, 0.06f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.48f, 0.29f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.20f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.04f, 0.08f, 0.04f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.35f, 0.55f, 0.35f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.45f, 0.65f, 0.45f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.55f, 0.75f, 0.55f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.70f, 1.00f, 0.70f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.60f, 0.95f, 0.60f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.70f, 1.00f, 0.70f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.40f, 0.80f, 0.40f, 0.60f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.90f, 0.50f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.75f, 0.35f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.40f, 0.80f, 0.40f, 0.45f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.50f, 0.90f, 0.50f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.45f, 0.85f, 0.45f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.30f, 0.50f, 0.30f, 0.60f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.50f, 0.80f, 0.50f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.55f, 0.85f, 0.55f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.40f, 0.80f, 0.40f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.50f, 0.90f, 0.50f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.55f, 0.95f, 0.55f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.25f, 0.50f, 0.25f, 0.86f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.50f, 0.90f, 0.50f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.35f, 0.70f, 0.35f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.15f, 0.08f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.40f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.20f, 0.35f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.35f, 0.55f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.45f, 0.28f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.50f, 0.90f, 0.50f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.50f, 0.90f, 0.50f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

void VerseFinderApp::run() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Handle keyboard shortcuts
        handleKeyboardShortcuts();
        
        // Create main window
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        
        ImGui::Begin("VerseFinder", nullptr, window_flags);
        ImGui::PopStyleVar(2);
        
        // Create menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Settings", "Ctrl+,")) {
                    show_settings_window = true;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4")) {
                    glfwSetWindowShouldClose(window, true);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Clear Search", "Ctrl+K")) {
                    clearSearch();
                }
                if (ImGui::MenuItem("Copy Verse", "Ctrl+C", false, !selected_verse_text.empty())) {
                    copyToClipboard(selected_verse_text);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Auto Search", nullptr, &auto_search);
                ImGui::MenuItem("Performance Stats", nullptr, &show_performance_stats);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("Help", "F1")) {
                    show_help_window = true;
                }
                if (ImGui::MenuItem("About")) {
                    show_about_window = true;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        
        // Main content
        renderMainWindow();
        
        ImGui::End();
        
        // Render modals and dialogs
        if (show_verse_modal) {
            renderVerseModal();
        }
        
        if (show_settings_window) {
            renderSettingsWindow();
        }
        
        if (show_about_window) {
            renderAboutWindow();
        }
        
        if (show_help_window) {
            renderHelpWindow();
        }
        
        if (show_performance_stats) {
            renderPerformanceWindow();
        }
        
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.11f, 0.11f, 0.12f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // Render presentation window if active
        if (isPresentationWindowActive()) {
            renderPresentationWindow();
        }
        
        glfwSwapBuffers(window);
    }
}

void VerseFinderApp::renderMainWindow() {
    ImGui::Columns(2, "main_columns", true);
    static bool first_time = true;
    if (first_time) {
        ImGui::SetColumnWidth(0, 400.0f);
        first_time = false;
    }
    
    // Left panel - Search and results
    ImGui::BeginChild("SearchPanel", ImVec2(0, 0), true);
    
    renderSearchArea();
    ImGui::Separator();
    renderSearchResults();
    
    ImGui::EndChild();
    
    ImGui::NextColumn();
    
    // Right panel - Translation info and status
    ImGui::BeginChild("InfoPanel", ImVec2(0, 0), true);
    
    renderTranslationSelector();
    ImGui::Separator();
    renderPresentationPreview();
    ImGui::Separator();
    renderStatusBar();
    
    ImGui::EndChild();
}

void VerseFinderApp::renderSearchArea() {
    ImGui::Text("Bible Search");
    ImGui::Spacing();
    
    // Search input
    ImGui::PushItemWidth(-1);
    bool search_changed = ImGui::InputTextWithHint("##search", "Enter verse reference (e.g., 'John 3:16') or keywords...", 
                                                  search_input, sizeof(search_input), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopItemWidth();
    
    // Search history dropdown (if history exists)
    if (!userSettings.content.searchHistory.empty()) {
        ImGui::Spacing();
        if (ImGui::BeginCombo("Recent Searches", nullptr)) {
            for (const auto& historical_search : userSettings.content.searchHistory) {
                if (ImGui::Selectable(historical_search.c_str())) {
                    strncpy(search_input, historical_search.c_str(), sizeof(search_input) - 1);
                    search_input[sizeof(search_input) - 1] = '\0';
                    performSearch();
                }
            }
            ImGui::EndCombo();
        }
    }
    
    // Auto-search or manual search
    if (search_changed || (auto_search && strcmp(search_input, last_search_query.c_str()) != 0)) {
        last_search_query = search_input;
        performSearch();
    }
    
    // Search buttons and controls
    ImGui::Spacing();
    if (ImGui::Button("Search", ImVec2(100, 0))) {
        performSearch();
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear", ImVec2(80, 0))) {
        clearSearch();
    }
    ImGui::SameLine();
    ImGui::Text("Auto: ");
    ImGui::SameLine();
    ImGui::Checkbox("##auto_search", &auto_search);
    
    // Fuzzy search controls
    ImGui::SameLine();
    ImGui::Text("Fuzzy: ");
    ImGui::SameLine();
    if (ImGui::Checkbox("##fuzzy_search", &fuzzy_search_enabled)) {
        bible.enableFuzzySearch(fuzzy_search_enabled);
        // Generate suggestions if enabling fuzzy search and there's input
        if (fuzzy_search_enabled && strlen(search_input) > 0) {
            query_suggestions = bible.generateQuerySuggestions(search_input, current_translation.name);
            book_suggestions = bible.findBookNameSuggestions(search_input);
        }
    }
    
    // Show fuzzy search suggestions
    if (fuzzy_search_enabled && strlen(search_input) > 0) {
        // Book name suggestions
        if (!book_suggestions.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "Did you mean:");
            for (size_t i = 0; i < std::min(book_suggestions.size(), size_t(3)); ++i) {
                const auto& suggestion = book_suggestions[i];
                std::string confidence_text = "";
                if (suggestion.matchType == "fuzzy") {
                    confidence_text = " (~" + std::to_string(static_cast<int>(suggestion.confidence * 100)) + "%)";
                } else if (suggestion.matchType == "phonetic") {
                    confidence_text = " (phonetic)";
                } else if (suggestion.matchType == "partial") {
                    confidence_text = " (...)";
                }
                
                if (ImGui::SmallButton((suggestion.text + confidence_text).c_str())) {
                    strncpy(search_input, suggestion.text.c_str(), sizeof(search_input) - 1);
                    search_input[sizeof(search_input) - 1] = '\0';
                    performSearch();
                }
                if (i < std::min(book_suggestions.size(), size_t(3)) - 1) {
                    ImGui::SameLine();
                }
            }
        }
        
        // Query keyword suggestions
        if (!query_suggestions.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "Suggestions:");
            for (size_t i = 0; i < std::min(query_suggestions.size(), size_t(3)); ++i) {
                if (ImGui::SmallButton(query_suggestions[i].c_str())) {
                    strncpy(search_input, query_suggestions[i].c_str(), sizeof(search_input) - 1);
                    search_input[sizeof(search_input) - 1] = '\0';
                    performSearch();
                }
                if (i < std::min(query_suggestions.size(), size_t(3)) - 1) {
                    ImGui::SameLine();
                }
            }
        }
    }
    
    // Search hints
    if (strlen(search_input) == 0) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Examples:");
        ImGui::BulletText("John 3:16 - Find specific verse");
        ImGui::BulletText("love - Find verses with keyword");
        ImGui::BulletText("faith hope love - Find multiple keywords");
        ImGui::BulletText("Psalm 23 - Find chapter references");
        
        if (fuzzy_search_enabled) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "Fuzzy Search Examples:");
            ImGui::BulletText("Jhn 3:16 - Corrects typos in book names");
            ImGui::BulletText("luv - Finds 'love' with phonetic matching");
            ImGui::BulletText("Gen - Suggests 'Genesis' from partial match");
            ImGui::BulletText("fait - Suggests 'faith' from similar spelling");
        }
    }
}

void VerseFinderApp::renderSearchResults() {
    if (!bible.isReady()) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Loading Bible data...");
        return;
    }
    
    if (search_results.empty()) {
        if (strlen(search_input) > 0) {
            ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.4f, 1.0f), "No verses found");
            ImGui::Text("Try different keywords or check the reference");
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Enter search terms above");
        }
        return;
    }
    
    // Show different headers based on search type
    if (is_viewing_chapter) {
        ImGui::Text("%s Chapter %d (%d verses)", 
                   current_chapter_book.c_str(), current_chapter_number, (int)search_results.size());
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click any verse to jump to it");
    } else {
        ImGui::Text("Results (%d found)", (int)search_results.size());
    }
    ImGui::Separator();
    
    // Results list with scrolling
    ImGui::BeginChild("ResultsList", ImVec2(0, 0), false);
    
    for (size_t i = 0; i < search_results.size(); ++i) {
        const std::string& result = search_results[i];
        
        // Parse reference and text
        size_t colon_pos = result.find(": ");
        if (colon_pos == std::string::npos) continue;
        
        std::string reference = result.substr(0, colon_pos);
        std::string verse_text = result.substr(colon_pos + 2);
        
        // Highlight current selection
        bool is_selected = (static_cast<int>(i) == selected_result_index);
        if (is_selected) {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.3f, 0.5f, 0.8f, 0.3f));
        }
        
        // Different layout for chapter viewing vs search results
        float child_height = is_viewing_chapter ? 60.0f : 80.0f;
        ImGui::BeginChild(("result_" + std::to_string(i)).c_str(), ImVec2(0, child_height), true);
        
        if (is_viewing_chapter) {
            // Extract verse number for chapter viewing
            size_t last_colon = reference.find_last_of(':');
            std::string verse_num = (last_colon != std::string::npos) ? 
                                   reference.substr(last_colon + 1) : std::to_string(i + 1);
            
            // Show verse number prominently and make it clickable
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.9f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.3f, 0.7f, 1.0f));
            
            if (ImGui::Button(("v" + verse_num).c_str(), ImVec2(40, 0))) {
                // Jump to this specific verse
                std::string book;
                int chapter, verse;
                if (bible.parseReference(reference, book, chapter, verse)) {
                    jumpToVerse(book, chapter, verse);
                }
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            ImGui::Text("%s", verse_text.c_str());
        } else {
            // Regular search result display
            // Show favorite star if this verse is favorited
            bool isFavorite = userSettings.isFavoriteVerse(result);
            if (isFavorite) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "*");
                ImGui::SameLine();
            }
            ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "%s", reference.c_str());
            
            // Verse text with word wrapping
            ImGui::PushTextWrapPos(0.0f);
            
            // Highlight search terms in verse text
            std::string display_text = verse_text;
            if (display_text.length() > 150) {
                display_text = display_text.substr(0, 147) + "...";
            }
            
            // Simple highlighting with case-insensitive search
            bool should_highlight = false;
            if (strlen(search_input) > 0) {
                std::string lower_search = search_input;
                std::string lower_display = display_text;
                std::transform(lower_search.begin(), lower_search.end(), lower_search.begin(),
                              [](unsigned char c){ return std::tolower(c); });
                std::transform(lower_display.begin(), lower_display.end(), lower_display.begin(),
                              [](unsigned char c){ return std::tolower(c); });
                should_highlight = lower_display.find(lower_search) != std::string::npos;
            }
            
            if (should_highlight) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.6f, 1.0f), "%s", display_text.c_str());
            } else {
                ImGui::Text("%s", display_text.c_str());
            }
            
            ImGui::PopTextWrapPos();
        }
        
        // Click to select/view
        if (ImGui::IsItemClicked()) {
            selectResult(static_cast<int>(i));
        }
        
        // Double-click to open modal
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            selectResult(static_cast<int>(i));
            show_verse_modal = true;
        }
        
        // Right-click context menu
        if (ImGui::BeginPopupContextItem(("context_" + std::to_string(i)).c_str())) {
            bool isFavorite = userSettings.isFavoriteVerse(result);
            if (isFavorite) {
                if (ImGui::MenuItem("Remove from Favorites")) {
                    userSettings.removeFavoriteVerse(result);
                }
            } else {
                if (ImGui::MenuItem("Add to Favorites")) {
                    userSettings.addFavoriteVerse(result);
                }
            }
            if (ImGui::MenuItem("Copy to Clipboard")) {
                copyToClipboard(result);
            }
            if (ImGui::MenuItem("View Full Verse")) {
                selectResult(static_cast<int>(i));
                show_verse_modal = true;
            }
            if (userSettings.presentation.enabled && ImGui::MenuItem("Display on Presentation")) {
                selectResult(static_cast<int>(i));
                std::string verse_text = formatVerseText(selected_verse_text);
                std::string reference = formatVerseReference(selected_verse_text);
                displayVerseOnPresentation(verse_text, reference);
            }
            ImGui::EndPopup();
        }
        
        ImGui::EndChild();
        
        if (is_selected) {
            ImGui::PopStyleColor();
        }
    }
    
    ImGui::EndChild();
}

void VerseFinderApp::renderTranslationSelector() {
    ImGui::Text("📚 Translation");
    
    if (!bible.isReady()) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Loading...");
        return;
    }
    
    const auto& translations = bible.getTranslations();
    if (translations.empty()) {
        ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.4f, 1.0f), "No translations loaded");
        if (ImGui::Button("Open Settings")) {
            show_settings_window = true;
        }
        return;
    }
    
    // Current translation display
    if (!current_translation.name.empty()) {
        ImGui::Text("Current: %s (%s)", current_translation.name.c_str(), current_translation.abbreviation.c_str());
    }
    
    ImGui::Spacing();
    
    // Translation selector
    if (ImGui::BeginCombo("##translation", current_translation.abbreviation.c_str())) {
        for (const auto& trans : translations) {
            bool is_selected = (current_translation.name == trans.name);
            if (ImGui::Selectable((trans.name + " (" + trans.abbreviation + ")").c_str(), is_selected)) {
                switchToTranslation(trans.name);
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    
    if (ImGui::Button("Manage Translations", ImVec2(-1, 0))) {
        show_settings_window = true;
    }
}

void VerseFinderApp::renderStatusBar() {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Status");
    
    if (bible.isReady()) {
        const auto& translations = bible.getTranslations();
        ImGui::Text("Ready - %d translation(s) loaded", (int)translations.size());
        
        if (!search_results.empty()) {
            ImGui::Text("Found %d verse(s)", (int)search_results.size());
            if (selected_result_index >= 0) {
                ImGui::Text("Selected: %d", selected_result_index + 1);
            }
        }
        
        // Performance information
        if (last_search_time_ms > 0.0) {
            ImGui::Text("Search: %.2f ms", last_search_time_ms);
        }
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Loading Bible data...");
    }
    
    // Selected verse preview
    if (!selected_verse_text.empty()) {
        ImGui::Spacing();
        ImGui::Text("Selected Verse:");
        ImGui::Separator();
        
        std::string reference = formatVerseReference(selected_verse_text);
        std::string verse_text = formatVerseText(selected_verse_text);
        
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "%s", reference.c_str());
        
        ImGui::PushTextWrapPos(0.0f);
        ImGui::Text("%s", verse_text.c_str());
        ImGui::PopTextWrapPos();
        
        if (ImGui::Button("View Full", ImVec2(-1, 0))) {
            show_verse_modal = true;
        }
    }
}

void VerseFinderApp::renderVerseModal() {
    ImGui::SetNextWindowSize(ImVec2(900, 650), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Verse Details", &show_verse_modal, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (!selected_verse_text.empty()) {
            std::string reference = formatVerseReference(selected_verse_text);
            std::string verse_text = formatVerseText(selected_verse_text);
            
            // Reference header with larger font
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
            ImGui::SetWindowFontScale(1.4f);
            ImGui::Text("%s", reference.c_str());
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopStyleColor();
            
            ImGui::Separator();
            ImGui::Spacing();
            
            // Verse text with larger font and better spacing
            ImGui::PushTextWrapPos(0.0f);
            ImGui::SetWindowFontScale(1.2f);
            ImGui::TextWrapped("%s", verse_text.c_str());
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopTextWrapPos();
            
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            // Enhanced navigation buttons
            if (ImGui::Button("-10", ImVec2(70, 35))) {
                navigateToVerse(-10);
            }
            ImGui::SameLine();
            if (ImGui::Button("-1", ImVec2(60, 35))) {
                navigateToVerse(-1);
            }
            ImGui::SameLine();
            if (ImGui::Button("+1", ImVec2(60, 35))) {
                navigateToVerse(1);
            }
            ImGui::SameLine();
            if (ImGui::Button("+10", ImVec2(70, 35))) {
                navigateToVerse(10);
            }
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();
            
            // Action buttons
            if (ImGui::Button("Copy", ImVec2(100, 35))) {
                copyToClipboard(selected_verse_text);
            }
            ImGui::SameLine();
            if (ImGui::Button("Close", ImVec2(100, 35))) {
                show_verse_modal = false;
            }
        }
    }
    ImGui::End();
}

void VerseFinderApp::renderSettingsWindow() {
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Settings", &show_settings_window)) {
        if (ImGui::BeginTabBar("SettingsTabs")) {
            if (ImGui::BeginTabItem("📚 Translations")) {
                ImGui::Text("Manage Bible translations for VerseFinder");
                ImGui::Separator();
                
                // Download all button
                if (ImGui::Button("Download All Free Translations", ImVec2(-1, 30))) {
                    for (auto& trans : available_translations) {
                        if (!trans.is_downloaded && !trans.is_downloading) {
                            downloadTranslation(trans.url, trans.name);
                        }
                    }
                }
                
                ImGui::Spacing();
                
                // Translation list
                if (ImGui::BeginTable("TranslationsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("Translation");
                    ImGui::TableSetupColumn("Status");
                    ImGui::TableSetupColumn("Description");
                    ImGui::TableSetupColumn("Actions");
                    ImGui::TableHeadersRow();
                    
                    for (auto& trans : available_translations) {
                        ImGui::TableNextRow();
                        
                        // Name
                        ImGui::TableNextColumn();
                        ImGui::Text("%s (%s)", trans.name.c_str(), trans.abbreviation.c_str());
                        
                        // Status
                        ImGui::TableNextColumn();
                        if (trans.is_downloading) {
                            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Downloading...");
                            ImGui::ProgressBar(trans.download_progress);
                        } else if (trans.is_downloaded) {
                            ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Downloaded");
                        } else {
                            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Available");
                        }
                        
                        // Description
                        ImGui::TableNextColumn();
                        ImGui::TextWrapped("%s", trans.description.c_str());
                        
                        // Actions
                        ImGui::TableNextColumn();
                        std::string button_id = "##" + trans.abbreviation;
                        
                        if (trans.is_downloaded) {
                            if (ImGui::Button(("Select" + button_id).c_str(), ImVec2(80, 0))) {
                                switchToTranslation(trans.name);
                            }
                        } else if (!trans.is_downloading) {
                            if (ImGui::Button(("Download" + button_id).c_str(), ImVec2(80, 0))) {
                                downloadTranslation(trans.url, trans.name);
                            }
                        }
                    }
                    
                    ImGui::EndTable();
                }
                
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("🎨 Appearance")) {
                ImGui::Text("Customize the appearance of VerseFinder");
                ImGui::Separator();
                
                // Font Settings
                ImGui::Text("🔤 Font Settings");
                ImGui::Spacing();
                
                // Font size slider
                float fontSize = userSettings.display.fontSize;
                if (ImGui::SliderFloat("Font Size", &fontSize, 8.0f, 36.0f, "%.1f")) {
                    userSettings.display.fontSize = fontSize;
                    // Apply font size change immediately
                    ImGui::GetIO().FontGlobalScale = fontSize / 16.0f; // Relative to default 16px
                }
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Adjust text size for better readability");
                
                ImGui::Spacing();
                ImGui::Separator();
                
                // Theme Settings
                ImGui::Text("🎨 Color Theme");
                ImGui::Spacing();
                
                // Theme selection
                const char* themes[] = { "Dark", "Light", "Blue", "Green" };
                int currentTheme = 0;
                if (userSettings.display.colorTheme == "light") currentTheme = 1;
                else if (userSettings.display.colorTheme == "blue") currentTheme = 2;
                else if (userSettings.display.colorTheme == "green") currentTheme = 3;
                
                if (ImGui::Combo("Color Theme", &currentTheme, themes, IM_ARRAYSIZE(themes))) {
                    switch (currentTheme) {
                        case 0: userSettings.display.colorTheme = "dark"; break;
                        case 1: userSettings.display.colorTheme = "light"; break;
                        case 2: userSettings.display.colorTheme = "blue"; break;
                        case 3: userSettings.display.colorTheme = "green"; break;
                    }
                    setupImGuiStyle(); // Apply theme immediately
                }
                
                ImGui::Spacing();
                
                // Color customization
                ImVec4 highlightColor = ImVec4(1.0f, 0.84f, 0.0f, 1.0f); // Default gold
                if (ImGui::ColorEdit3("Highlight Color", (float*)&highlightColor)) {
                    char colorHex[16];
                    sprintf(colorHex, "#%02X%02X%02X", 
                           (int)(highlightColor.x * 255), 
                           (int)(highlightColor.y * 255), 
                           (int)(highlightColor.z * 255));
                    userSettings.display.highlightColor = colorHex;
                }
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Color used for highlighting search results");
                
                ImGui::Spacing();
                ImGui::Separator();
                
                // Window Settings
                ImGui::Text("Window Settings");
                ImGui::Spacing();
                
                bool rememberWindow = userSettings.display.rememberWindowState;
                if (ImGui::Checkbox("Remember Window Size & Position", &rememberWindow)) {
                    userSettings.display.rememberWindowState = rememberWindow;
                }
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Save window layout between sessions");
                
                if (userSettings.display.rememberWindowState) {
                    ImGui::Spacing();
                    ImGui::Text("Current window: %dx%d", userSettings.display.windowWidth, userSettings.display.windowHeight);
                    if (ImGui::Button("Reset Window Size")) {
                        glfwSetWindowSize(window, 1400, 900);
                        glfwSetWindowPos(window, 100, 100);
                    }
                }
                
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Search")) {
                ImGui::Text("Configure search behavior and preferences");
                ImGui::Separator();
                
                // General Search Settings
                ImGui::Text("General Search Settings");
                ImGui::Spacing();
                
                // Default translation
                if (ImGui::BeginCombo("Default Translation", userSettings.search.defaultTranslation.c_str())) {
                    for (const auto& trans : bible.getTranslations()) {
                        bool isSelected = (userSettings.search.defaultTranslation == trans.abbreviation);
                        if (ImGui::Selectable(trans.abbreviation.c_str(), isSelected)) {
                            userSettings.search.defaultTranslation = trans.abbreviation;
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Translation to use when app starts");
                
                ImGui::Spacing();
                
                // Max search results
                int maxResults = userSettings.search.maxSearchResults;
                if (ImGui::SliderInt("Max Search Results", &maxResults, 10, 200)) {
                    userSettings.search.maxSearchResults = maxResults;
                }
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Maximum number of verses to show in search results");
                
                ImGui::Spacing();
                
                // Auto search
                bool autoSearchSetting = userSettings.search.autoSearch;
                if (ImGui::Checkbox("Auto Search", &autoSearchSetting)) {
                    userSettings.search.autoSearch = autoSearchSetting;
                    auto_search = autoSearchSetting;
                }
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Search automatically as you type");
                
                ImGui::Spacing();
                
                // Search result format
                const char* formats[] = { "Reference + Text", "Text Only", "Reference Only" };
                int currentFormat = 0;
                if (userSettings.search.searchResultFormat == "text_only") currentFormat = 1;
                else if (userSettings.search.searchResultFormat == "reference_only") currentFormat = 2;
                
                if (ImGui::Combo("Search Result Format", &currentFormat, formats, IM_ARRAYSIZE(formats))) {
                    switch (currentFormat) {
                        case 0: userSettings.search.searchResultFormat = "reference_text"; break;
                        case 1: userSettings.search.searchResultFormat = "text_only"; break;
                        case 2: userSettings.search.searchResultFormat = "reference_only"; break;
                    }
                }
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "How to display search results");
                
                ImGui::Spacing();
                
                // Performance stats
                bool showPerfStats = userSettings.search.showPerformanceStats;
                if (ImGui::Checkbox("Show Performance Statistics", &showPerfStats)) {
                    userSettings.search.showPerformanceStats = showPerfStats;
                    show_performance_stats = showPerfStats;
                }
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Display search timing information");
                
                ImGui::Spacing();
                ImGui::Separator();
                
                // Fuzzy search settings
                ImGui::Text("Fuzzy Search Settings");
                ImGui::Spacing();
                
                bool fuzzyEnabled = userSettings.search.fuzzySearchEnabled;
                if (ImGui::Checkbox("Enable Fuzzy Search", &fuzzyEnabled)) {
                    userSettings.search.fuzzySearchEnabled = fuzzyEnabled;
                    fuzzy_search_enabled = fuzzyEnabled;
                    bible.enableFuzzySearch(fuzzy_search_enabled);
                }
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Find verses even with typos and approximate matches");
                
                if (fuzzy_search_enabled) {
                    ImGui::Spacing();
                    
                    // Get current fuzzy search options
                    FuzzySearchOptions options = bible.getFuzzySearchOptions();
                    bool options_changed = false;
                    
                    // Minimum confidence slider
                    float min_confidence = static_cast<float>(options.minConfidence);
                    if (ImGui::SliderFloat("Similarity Threshold", &min_confidence, 0.1f, 1.0f, "%.2f")) {
                        options.minConfidence = static_cast<double>(min_confidence);
                        options_changed = true;
                    }
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Higher values = more strict matching");
                    
                    // Max edit distance
                    int max_edit_distance = options.maxEditDistance;
                    if (ImGui::SliderInt("Max Edit Distance", &max_edit_distance, 1, 5)) {
                        options.maxEditDistance = max_edit_distance;
                        options_changed = true;
                    }
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Maximum character changes allowed");
                    
                    // Phonetic matching
                    if (ImGui::Checkbox("Enable Phonetic Matching", &options.enablePhonetic)) {
                        options_changed = true;
                    }
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Match words that sound similar (e.g., 'John' ~ 'Jon')");
                    
                    // Partial matching
                    if (ImGui::Checkbox("Enable Partial Matching", &options.enablePartialMatch)) {
                        options_changed = true;
                    }
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Match partial words and substrings");
                    
                    // Max suggestions
                    int max_suggestions = options.maxSuggestions;
                    if (ImGui::SliderInt("Max Suggestions", &max_suggestions, 3, 10)) {
                        options.maxSuggestions = max_suggestions;
                        options_changed = true;
                    }
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Maximum number of suggestions to show");
                    
                    // Apply changes
                    if (options_changed) {
                        bible.setFuzzySearchOptions(options);
                    }
                    
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Text("Fuzzy Search Examples:");
                    ImGui::BulletText("'Jhn 3:16' → 'John 3:16' (typo correction)");
                    ImGui::BulletText("'luv' → verses about 'love' (phonetic matching)");
                    ImGui::BulletText("'Gen' → 'Genesis' (partial matching)");
                    ImGui::BulletText("'fait' → 'faith' (edit distance matching)");
                }
                
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("📚 Content")) {
                ImGui::Text("Manage favorites, history, and content preferences");
                ImGui::Separator();
                
                // Favorites section
                ImGui::Text("Favorite Verses");
                ImGui::Spacing();
                
                ImGui::Text("Saved favorites: %zu", userSettings.content.favoriteVerses.size());
                
                if (ImGui::BeginListBox("##FavoritesList", ImVec2(-1, 120))) {
                    for (size_t i = 0; i < userSettings.content.favoriteVerses.size(); i++) {
                        const std::string& verse = userSettings.content.favoriteVerses[i];
                        bool selected = false;
                        if (ImGui::Selectable(verse.c_str(), selected)) {
                            // Could add functionality to jump to this verse
                        }
                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                            // Remove from favorites on double-click
                            userSettings.removeFavoriteVerse(verse);
                        }
                    }
                    ImGui::EndListBox();
                }
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Double-click to remove from favorites");
                
                ImGui::Spacing();
                ImGui::Separator();
                
                // Search History section
                ImGui::Text("🕒 Search History");
                ImGui::Spacing();
                
                bool saveHistory = userSettings.content.saveSearchHistory;
                if (ImGui::Checkbox("Save Search History", &saveHistory)) {
                    userSettings.content.saveSearchHistory = saveHistory;
                }
                
                if (userSettings.content.saveSearchHistory) {
                    ImGui::Spacing();
                    
                    int maxHistory = userSettings.content.maxHistoryEntries;
                    if (ImGui::SliderInt("Max History Entries", &maxHistory, 10, 500)) {
                        userSettings.content.maxHistoryEntries = maxHistory;
                    }
                    
                    ImGui::Spacing();
                    ImGui::Text("Recent searches: %zu", userSettings.content.searchHistory.size());
                    
                    if (ImGui::BeginListBox("##HistoryList", ImVec2(-1, 100))) {
                        for (const auto& search : userSettings.content.searchHistory) {
                            ImGui::Selectable(search.c_str(), false);
                        }
                        ImGui::EndListBox();
                    }
                    
                    if (ImGui::Button("Clear History")) {
                        userSettings.content.searchHistory.clear();
                    }
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                
                // Verse Display Format
                ImGui::Text("📝 Verse Display");
                ImGui::Spacing();
                
                const char* displayFormats[] = { "Standard", "Compact", "Detailed" };
                int currentDisplayFormat = 0;
                if (userSettings.content.verseDisplayFormat == "compact") currentDisplayFormat = 1;
                else if (userSettings.content.verseDisplayFormat == "detailed") currentDisplayFormat = 2;
                
                if (ImGui::Combo("Verse Display Format", &currentDisplayFormat, displayFormats, IM_ARRAYSIZE(displayFormats))) {
                    switch (currentDisplayFormat) {
                        case 0: userSettings.content.verseDisplayFormat = "standard"; break;
                        case 1: userSettings.content.verseDisplayFormat = "compact"; break;
                        case 2: userSettings.content.verseDisplayFormat = "detailed"; break;
                    }
                }
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "How verses are formatted when displayed");
                
                ImGui::Spacing();
                ImGui::Separator();
                
                // Recent Translations
                ImGui::Text("🔄 Recent Translations");
                ImGui::Spacing();
                
                ImGui::Text("Recently used: %zu", userSettings.content.recentTranslations.size());
                for (const auto& trans : userSettings.content.recentTranslations) {
                    ImGui::BulletText("%s", trans.c_str());
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                
                // Quick Stats and Actions
                ImGui::Text("Quick Stats");
                ImGui::Spacing();
                
                ImGui::Text("Total favorites: %zu", userSettings.content.favoriteVerses.size());
                ImGui::Text("Search history entries: %zu", userSettings.content.searchHistory.size());
                ImGui::Text("Recent translations: %zu", userSettings.content.recentTranslations.size());
                
                ImGui::Spacing();
                
                if (ImGui::Button("Clear All History")) {
                    userSettings.content.searchHistory.clear();
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear Favorites")) {
                    userSettings.content.favoriteVerses.clear();
                }
                
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Presentation")) {
                ImGui::Text("Configure presentation mode for OBS and live display");
                ImGui::Separator();
                
                // Enable/Disable presentation mode
                bool presentation_enabled = userSettings.presentation.enabled;
                if (ImGui::Checkbox("Enable Presentation Mode", &presentation_enabled)) {
                    userSettings.presentation.enabled = presentation_enabled;
                }
                
                if (userSettings.presentation.enabled) {
                    ImGui::Spacing();
                    
                    // Monitor selection
                    auto monitors = getAvailableMonitors();
                    if (!monitors.empty()) {
                        std::vector<const char*> monitor_names;
                        for (size_t i = 0; i < monitors.size(); i++) {
                            const char* name = glfwGetMonitorName(monitors[i]);
                            monitor_names.push_back(name ? name : ("Monitor " + std::to_string(i)).c_str());
                        }
                        
                        if (ImGui::Combo("Target Monitor", &userSettings.presentation.monitorIndex, 
                                       monitor_names.data(), static_cast<int>(monitor_names.size()))) {
                            if (presentation_mode_active) {
                                updatePresentationMonitorPosition();
                            }
                        }
                    }
                    
                    // Fullscreen option
                    bool fullscreen = userSettings.presentation.fullscreen;
                    if (ImGui::Checkbox("Fullscreen Mode", &fullscreen)) {
                        userSettings.presentation.fullscreen = fullscreen;
                        if (presentation_mode_active) {
                            updatePresentationMonitorPosition();
                        }
                    }
                    
                    if (!userSettings.presentation.fullscreen) {
                        // Window size settings
                        int width = userSettings.presentation.windowWidth;
                        int height = userSettings.presentation.windowHeight;
                        
                        if (ImGui::InputInt("Window Width", &width)) {
                            userSettings.presentation.windowWidth = std::max(640, width);
                        }
                        if (ImGui::InputInt("Window Height", &height)) {
                            userSettings.presentation.windowHeight = std::max(480, height);
                        }
                    }
                    
                    ImGui::Separator();
                    ImGui::Text("🎨 Appearance Settings");
                    ImGui::Spacing();
                    
                    // Font size
                    float fontSize = userSettings.presentation.fontSize;
                    if (ImGui::SliderFloat("Font Size", &fontSize, 24.0f, 120.0f, "%.0f")) {
                        userSettings.presentation.fontSize = fontSize;
                    }
                    
                    // Text alignment
                    const char* alignments[] = {"left", "center", "right"};
                    int current_alignment = 1; // default to center
                    if (userSettings.presentation.textAlignment == "left") current_alignment = 0;
                    else if (userSettings.presentation.textAlignment == "right") current_alignment = 2;
                    
                    if (ImGui::Combo("Text Alignment", &current_alignment, alignments, 3)) {
                        userSettings.presentation.textAlignment = alignments[current_alignment];
                    }
                    
                    // Text padding
                    float padding = userSettings.presentation.textPadding;
                    if (ImGui::SliderFloat("Text Padding", &padding, 10.0f, 100.0f, "%.0f")) {
                        userSettings.presentation.textPadding = padding;
                    }
                    
                    // Show reference option
                    bool showReference = userSettings.presentation.showReference;
                    if (ImGui::Checkbox("Show Bible Reference", &showReference)) {
                        userSettings.presentation.showReference = showReference;
                    }
                    
                    ImGui::Separator();
                    ImGui::Text("🎨 Colors");
                    ImGui::Spacing();
                    
                    // Background color
                    ImVec4 bg_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                    if (userSettings.presentation.backgroundColor.length() == 7 && userSettings.presentation.backgroundColor[0] == '#') {
                        std::string hex = userSettings.presentation.backgroundColor.substr(1);
                        bg_color.x = std::stoi(hex.substr(0, 2), 0, 16) / 255.0f;
                        bg_color.y = std::stoi(hex.substr(2, 2), 0, 16) / 255.0f;
                        bg_color.z = std::stoi(hex.substr(4, 2), 0, 16) / 255.0f;
                    }
                    
                    if (ImGui::ColorEdit3("Background Color", (float*)&bg_color)) {
                        char hex[8];
                        sprintf(hex, "#%02X%02X%02X", 
                               (int)(bg_color.x * 255), 
                               (int)(bg_color.y * 255), 
                               (int)(bg_color.z * 255));
                        userSettings.presentation.backgroundColor = std::string(hex);
                    }
                    
                    // Text color
                    ImVec4 text_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                    if (userSettings.presentation.textColor.length() == 7 && userSettings.presentation.textColor[0] == '#') {
                        std::string hex = userSettings.presentation.textColor.substr(1);
                        text_color.x = std::stoi(hex.substr(0, 2), 0, 16) / 255.0f;
                        text_color.y = std::stoi(hex.substr(2, 2), 0, 16) / 255.0f;
                        text_color.z = std::stoi(hex.substr(4, 2), 0, 16) / 255.0f;
                    }
                    
                    if (ImGui::ColorEdit3("Text Color", (float*)&text_color)) {
                        char hex[8];
                        sprintf(hex, "#%02X%02X%02X", 
                               (int)(text_color.x * 255), 
                               (int)(text_color.y * 255), 
                               (int)(text_color.z * 255));
                        userSettings.presentation.textColor = std::string(hex);
                    }
                    
                    // Reference color
                    ImVec4 ref_color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
                    if (userSettings.presentation.referenceColor.length() == 7 && userSettings.presentation.referenceColor[0] == '#') {
                        std::string hex = userSettings.presentation.referenceColor.substr(1);
                        ref_color.x = std::stoi(hex.substr(0, 2), 0, 16) / 255.0f;
                        ref_color.y = std::stoi(hex.substr(2, 2), 0, 16) / 255.0f;
                        ref_color.z = std::stoi(hex.substr(4, 2), 0, 16) / 255.0f;
                    }
                    
                    if (ImGui::ColorEdit3("Reference Color", (float*)&ref_color)) {
                        char hex[8];
                        sprintf(hex, "#%02X%02X%02X", 
                               (int)(ref_color.x * 255), 
                               (int)(ref_color.y * 255), 
                               (int)(ref_color.z * 255));
                        userSettings.presentation.referenceColor = std::string(hex);
                    }
                    
                    ImGui::Separator();
                    ImGui::Text("Advanced Options");
                    ImGui::Spacing();
                    
                    // OBS optimization
                    bool obsOptimized = userSettings.presentation.obsOptimized;
                    if (ImGui::Checkbox("OBS Studio Optimization", &obsOptimized)) {
                        userSettings.presentation.obsOptimized = obsOptimized;
                    }
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Optimizes window for OBS capture");
                    
                    // Auto hide cursor
                    bool autoHideCursor = userSettings.presentation.autoHideCursor;
                    if (ImGui::Checkbox("Auto-hide Cursor", &autoHideCursor)) {
                        userSettings.presentation.autoHideCursor = autoHideCursor;
                    }
                    
                    // Fade transition time
                    float fadeTime = userSettings.presentation.fadeTransitionTime;
                    if (ImGui::SliderFloat("Fade Transition Time", &fadeTime, 0.0f, 2.0f, "%.1fs")) {
                        userSettings.presentation.fadeTransitionTime = fadeTime;
                    }
                    
                    // Window title for OBS
                    char title[256];
                    strncpy(title, userSettings.presentation.windowTitle.c_str(), sizeof(title) - 1);
                    title[sizeof(title) - 1] = '\0';
                    
                    if (ImGui::InputText("Window Title", title, sizeof(title))) {
                        userSettings.presentation.windowTitle = std::string(title);
                    }
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Title shown in OBS window capture list");
                }
                
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Shortcuts")) {
                ImGui::Text("Keyboard shortcuts for VerseFinder");
                ImGui::Separator();
                
                ImGui::BulletText("Ctrl+K - Clear search");
                ImGui::BulletText("Ctrl+C - Copy selected verse");
                ImGui::BulletText("Ctrl+P - Performance statistics");
                ImGui::BulletText("Ctrl+, - Open settings");
                ImGui::BulletText("F1 - Show help");
                ImGui::BulletText("F5 - Toggle presentation mode");
                ImGui::BulletText("F6 - Toggle blank screen (presentation)");
                ImGui::BulletText("F7 - Display selected verse (presentation)");
                ImGui::BulletText("Enter - Search");
                ImGui::BulletText("Escape - Close dialogs");
                
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
        
        ImGui::Separator();
        
        // Settings management buttons
        if (ImGui::Button("💾 Save Settings", ImVec2(120, 0))) {
            saveSettings();
        }
        ImGui::SameLine();
        
        if (ImGui::Button("📤 Export", ImVec2(120, 0))) {
            // Simple export to current directory for now
            std::string exportPath = getExecutablePath() + "/exported_settings.json";
            if (exportSettings(exportPath)) {
                std::cout << "Settings exported to: " << exportPath << std::endl;
            } else {
                std::cerr << "Failed to export settings" << std::endl;
            }
        }
        ImGui::SameLine();
        
        if (ImGui::Button("📥 Import", ImVec2(120, 0))) {
            // Simple import from current directory for now
            std::string importPath = getExecutablePath() + "/exported_settings.json";
            if (importSettings(importPath)) {
                std::cout << "Settings imported from: " << importPath << std::endl;
                // Apply imported settings immediately
                fuzzy_search_enabled = userSettings.search.fuzzySearchEnabled;
                bible.enableFuzzySearch(fuzzy_search_enabled);
                auto_search = userSettings.search.autoSearch;
                show_performance_stats = userSettings.search.showPerformanceStats;
                setupImGuiStyle(); // Apply theme changes
            } else {
                std::cerr << "Failed to import settings or file not found" << std::endl;
            }
        }
        
        ImGui::Spacing();
        
        if (ImGui::Button("🔄 Reset to Defaults", ImVec2(150, 0))) {
            resetSettingsToDefault();
            // Apply reset settings immediately
            fuzzy_search_enabled = userSettings.search.fuzzySearchEnabled;
            bible.enableFuzzySearch(fuzzy_search_enabled);
            auto_search = userSettings.search.autoSearch;
            show_performance_stats = userSettings.search.showPerformanceStats;
            setupImGuiStyle(); // Apply theme changes
        }
        ImGui::SameLine();
        
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            show_settings_window = false;
        }
    }
    ImGui::End();
}

void VerseFinderApp::renderAboutWindow() {
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("About VerseFinder", &show_about_window, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("VerseFinder");
        ImGui::Text("Bible Search for Churches");
        ImGui::Separator();
        
        ImGui::Text("Version: 2.0 (ImGui Edition)");
        ImGui::Text("Built with Dear ImGui and C++");
        ImGui::Spacing();
        
        ImGui::Text("Features:");
        ImGui::BulletText("Fast verse lookup by reference");
        ImGui::BulletText("Keyword search across translations");
        ImGui::BulletText("Fuzzy search with typo correction");
        ImGui::BulletText("Multiple Bible translation support");
        ImGui::BulletText("Modern, responsive interface");
        ImGui::BulletText("Church-friendly design");
        
        ImGui::Spacing();
        ImGui::Separator();
        
        if (ImGui::Button("Close", ImVec2(-1, 0))) {
            show_about_window = false;
        }
    }
    ImGui::End();
}

void VerseFinderApp::renderHelpWindow() {
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("❓ Help", &show_help_window)) {
        ImGui::Text("How to Search");
        ImGui::Separator();
        
        ImGui::Text("Reference Search:");
        ImGui::BulletText("John 3:16 - Single verse");
        ImGui::BulletText("Psalm 23 - Entire chapter");
        ImGui::BulletText("Genesis 1:1-3 - Verse range");
        
        ImGui::Spacing();
        ImGui::Text("Keyword Search:");
        ImGui::BulletText("love - Find verses containing 'love'");
        ImGui::BulletText("faith hope love - Multiple keywords");
        ImGui::BulletText("\"for God so loved\" - Exact phrase");
        
        ImGui::Spacing();
        ImGui::Text("📚 Managing Translations");
        ImGui::Separator();
        ImGui::BulletText("Go to Settings > Translations");
        ImGui::BulletText("Download free translations");
        ImGui::BulletText("Switch between translations");
        
        ImGui::Spacing();
        ImGui::Text("Keyboard Shortcuts");
        ImGui::Separator();
        ImGui::BulletText("Ctrl+K - Clear search");
        ImGui::BulletText("Ctrl+C - Copy verse");
        ImGui::BulletText("Ctrl+P - Performance stats");
        ImGui::BulletText("Enter - Search");
        ImGui::BulletText("F1 - This help");
        
        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(-1, 0))) {
            show_help_window = false;
        }
    }
    ImGui::End();
}

void VerseFinderApp::renderPerformanceWindow() {
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("⚡ Performance Statistics", &show_performance_stats)) {
        ImGui::Text("Search Performance");
        ImGui::Separator();
        
        // Current search timing
        if (last_search_time_ms > 0.0) {
            ImGui::Text("Last Search Time: %.2f ms", last_search_time_ms);
            
            // Color-code performance
            ImVec4 color = ImVec4(0.3f, 0.8f, 0.3f, 1.0f); // Green
            if (last_search_time_ms > 50.0) {
                color = ImVec4(1.0f, 0.8f, 0.3f, 1.0f); // Yellow
            }
            if (last_search_time_ms > 100.0) {
                color = ImVec4(0.8f, 0.4f, 0.4f, 1.0f); // Red
            }
            
            ImGui::SameLine();
            ImGui::TextColored(color, "(Target: <50ms)");
        } else {
            ImGui::Text("No searches performed yet");
        }
        
        ImGui::Spacing();
        ImGui::Text("Cache Statistics");
        ImGui::Separator();
        
        // Get cache statistics from VerseFinder
        if (bible.isReady()) {
            // We need to print the performance stats which includes cache info
            std::string cache_info = "Cache information available in console";
            ImGui::Text("%s", cache_info.c_str());
            
            if (ImGui::Button("Print Full Stats to Console")) {
                bible.printPerformanceStats();
            }
            
            ImGui::Spacing();
            ImGui::Text("💾 Memory & System");
            ImGui::Separator();
            
            // Memory usage (if available)
            size_t memory_kb = PerformanceBenchmark::getCurrentMemoryUsage();
            if (memory_kb > 0) {
                ImGui::Text("Memory Usage: %.2f MB", memory_kb / 1024.0);
            } else {
                ImGui::Text("Memory Usage: Unable to determine");
            }
            
            ImGui::Spacing();
            ImGui::Text("Performance Targets");
            ImGui::Separator();
            
            ImGui::BulletText("Reference Search: < 5ms");
            ImGui::BulletText("Simple Keyword Search: < 20ms");
            ImGui::BulletText("Complex Multi-word Search: < 50ms");
            ImGui::BulletText("Cache Hit Rate: > 80%%");
            
            ImGui::Spacing();
            ImGui::Text("Cache Management");
            ImGui::Separator();
            
            if (ImGui::Button("Clear Search Cache")) {
                bible.clearSearchCache();
                ImGui::OpenPopup("Cache Cleared");
            }
            
            if (ImGui::BeginPopupModal("Cache Cleared", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Search cache has been cleared.");
                ImGui::Separator();
                if (ImGui::Button("OK", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Bible data still loading...");
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        
        if (ImGui::Button("Close", ImVec2(-1, 0))) {
            show_performance_stats = false;
        }
    }
    ImGui::End();
}

void VerseFinderApp::handleKeyboardShortcuts() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Ctrl+K - Clear search
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_K))) {
        clearSearch();
    }
    
    // Ctrl+C - Copy verse
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C)) && !selected_verse_text.empty()) {
        copyToClipboard(selected_verse_text);
    }
    
    // Ctrl+, - Settings
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Comma))) {
        show_settings_window = true;
    }
    
    // F1 - Help
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_F1))) {
        show_help_window = true;
    }
    
    // F5 - Toggle presentation mode
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_F5))) {
        togglePresentationMode();
    }
    
    // F6 - Toggle blank screen (if presentation is active)
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_F6)) && isPresentationWindowActive()) {
        toggleBlankScreen();
    }
    
    // F7 - Display selected verse on presentation (if one is selected)
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_F7)) && 
        isPresentationWindowActive() && !selected_verse_text.empty()) {
        std::string verse_text = formatVerseText(selected_verse_text);
        std::string reference = formatVerseReference(selected_verse_text);
        displayVerseOnPresentation(verse_text, reference);
    }
    
    // Ctrl+P - Performance stats
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_P))) {
        show_performance_stats = !show_performance_stats;
    }
    
    // Escape - Close modals
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
        show_verse_modal = false;
        show_settings_window = false;
        show_about_window = false;
        show_help_window = false;
        show_performance_stats = false;
    }
}

void VerseFinderApp::performSearch() {
    if (!bible.isReady() || strlen(search_input) == 0) {
        search_results.clear();
        selected_result_index = -1;
        selected_verse_text.clear();
        last_search_time_ms = 0.0;
        return;
    }
    
    std::string query = search_input;
    
    // Benchmark the search operation
    auto start_time = std::chrono::steady_clock::now();
    
    // Determine search type based on query format
    bool is_reference_format = false;
    
    // Check if query looks like a reference (contains numbers and potentially colons)
    std::regex reference_pattern(R"(^[a-zA-Z0-9\s]+\s+\d+(:?\d+)?$)");
    if (std::regex_match(query, reference_pattern)) {
        is_reference_format = true;
    }
    
    if (is_reference_format) {
        // Try exact verse reference first
        std::string ref_result = bible.searchByReference(query, current_translation.name);
        if (ref_result != "Verse not found." && ref_result != "Bible is loading...") {
            search_results = {query + ": " + ref_result};
            is_viewing_chapter = false;
        } else {
            // Try chapter search (e.g., "Hebrews 12")
            search_results = bible.searchByChapter(query, current_translation.name);
            
            // Check if this is a chapter search
            std::string book;
            int chapter, verse;
            if (bible.parseReference(query, book, chapter, verse) && chapter != -1 && verse == -1) {
                is_viewing_chapter = true;
                current_chapter_book = bible.normalizeBookName(book);
                current_chapter_number = chapter;
            } else {
                is_viewing_chapter = false;
            }
        }
    } else {
        // Use keyword search (with fuzzy search if enabled)
        if (fuzzy_search_enabled) {
            search_results = bible.searchByKeywordsFuzzy(query, current_translation.name);
            
            // Generate suggestions for the current query
            query_suggestions = bible.generateQuerySuggestions(query, current_translation.name);
            book_suggestions = bible.findBookNameSuggestions(query);
        } else {
            search_results = bible.searchByKeywords(query, current_translation.name);
        }
        is_viewing_chapter = false;
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    last_search_time_ms = duration.count() / 1000.0;
    
    // Apply search result limit
    if (search_results.size() > static_cast<size_t>(userSettings.search.maxSearchResults)) {
        search_results.resize(userSettings.search.maxSearchResults);
    }
    
    // Add to search history if enabled and results found
    if (!search_results.empty() && userSettings.content.saveSearchHistory) {
        userSettings.addToSearchHistory(query);
    }
    
    selected_result_index = search_results.empty() ? -1 : 0;
    if (selected_result_index >= 0) {
        selected_verse_text = search_results[selected_result_index];
    } else {
        selected_verse_text.clear();
    }
}

void VerseFinderApp::clearSearch() {
    memset(search_input, 0, sizeof(search_input));
    search_results.clear();
    selected_result_index = -1;
    selected_verse_text.clear();
    last_search_query.clear();
    last_search_time_ms = 0.0;
    
    // Clear fuzzy search suggestions
    query_suggestions.clear();
    book_suggestions.clear();
}

void VerseFinderApp::selectResult(int index) {
    if (index >= 0 && index < static_cast<int>(search_results.size())) {
        selected_result_index = index;
        selected_verse_text = search_results[index];
    }
}

void VerseFinderApp::copyToClipboard(const std::string& text) {
    if (window && !text.empty()) {
        glfwSetClipboardString(window, text.c_str());
        std::cout << "Successfully copied to clipboard: " << text.substr(0, 50) << (text.length() > 50 ? "..." : "") << std::endl;
    } else {
        std::cout << "Failed to copy to clipboard: " << (window ? "text is empty" : "no window available") << std::endl;
    }
}

std::string VerseFinderApp::formatVerseReference(const std::string& verse_text) {
    size_t colon_pos = verse_text.find(": ");
    return (colon_pos != std::string::npos) ? verse_text.substr(0, colon_pos) : "";
}

std::string VerseFinderApp::formatVerseText(const std::string& verse_text) {
    size_t colon_pos = verse_text.find(": ");
    return (colon_pos != std::string::npos) ? verse_text.substr(colon_pos + 2) : verse_text;
}

void VerseFinderApp::navigateToVerse(int direction) {
    if (selected_verse_text.empty()) return;
    
    // Parse current verse reference
    std::string reference = formatVerseReference(selected_verse_text);
    if (reference.empty()) return;
    
    // Use the improved navigation method from VerseFinder
    std::string result = bible.getAdjacentVerse(reference, current_translation.name, direction);
    
    if (!result.empty()) {
        selected_verse_text = result;
    }
}

void VerseFinderApp::jumpToVerse(const std::string& book, int chapter, int verse) {
    // Create the verse reference
    std::string reference = book + " " + std::to_string(chapter) + ":" + std::to_string(verse);
    
    // Search for this specific verse
    std::string result = bible.searchByReference(reference, current_translation.name);
    
    if (result != "Verse not found." && result != "Bible is loading...") {
        // Clear current search and show just this verse
        search_results = {reference + ": " + result};
        selected_result_index = 0;
        selected_verse_text = search_results[0];
        is_viewing_chapter = false;
        
        // Update search input to show the reference
        strncpy(search_input, reference.c_str(), sizeof(search_input) - 1);
        search_input[sizeof(search_input) - 1] = '\0';
    }
}

void VerseFinderApp::downloadTranslation(const std::string& url, const std::string& name) {
    // Mark as downloading
    for (auto& trans : available_translations) {
        if (trans.name == name) {
            trans.is_downloading = true;
            trans.download_progress = 0.0f;
            break;
        }
    }
    
    // Simulate download in a separate thread
    std::thread([this, url, name]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Update progress
        for (float progress = 0.0f; progress <= 1.0f; progress += 0.1f) {
            for (auto& trans : available_translations) {
                if (trans.name == name) {
                    trans.download_progress = progress;
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        
        try {
            // Get the correct filename based on URL and translation name
            std::string filename = extractFilenameFromUrl(url, name);
            
            // Try to find existing translation file in common locations first
            std::vector<std::string> search_paths = {
                getExecutablePath() + "/translations/" + filename,
                getExecutablePath() + "/" + filename,
                getExecutablePath() + "/data/" + filename,
                "./translations/" + filename,
                "./" + filename
            };
            
            std::string translation_content;
            bool found_existing = false;
            
            for (const auto& path : search_paths) {
                std::ifstream file(path);
                if (file.is_open()) {
                    std::stringstream buffer;
                    buffer << file.rdbuf();
                    translation_content = buffer.str();
                    found_existing = true;
                    std::cout << "Found existing translation at: " << path << std::endl;
                    break;
                }
            }
            
            if (!found_existing) {
                // Download from the provided URL
                std::cout << "Downloading translation from: " << url << std::endl;
                translation_content = downloadFromUrl(url);
                
                if (translation_content.empty()) {
                    throw std::runtime_error("Failed to download translation from URL: " + url);
                }
                
                std::cout << "Successfully downloaded " << name << " (" << translation_content.length() << " bytes)" << std::endl;
            }
            
            // Validate JSON format
            try {
                json test_parse = json::parse(translation_content);
                if (!test_parse.contains("translation") || !test_parse.contains("books")) {
                    throw std::runtime_error("Invalid Bible JSON format");
                }
            } catch (const json::exception& e) {
                throw std::runtime_error("Failed to parse translation JSON: " + std::string(e.what()));
            }
            
            // Save to translations directory and add to Bible
            if (bible.saveTranslation(translation_content, filename)) {
                bible.addTranslation(translation_content);
                
                // Mark as completed
                for (auto& trans : available_translations) {
                    if (trans.name == name) {
                        trans.is_downloading = false;
                        trans.is_downloaded = true;
                        trans.download_progress = 1.0f;
                        break;
                    }
                }
                
                updateAvailableTranslationStatus();
                std::cout << "Successfully downloaded and saved: " << name << std::endl;
            } else {
                throw std::runtime_error("Failed to save translation file");
            }
        } catch (const std::exception& e) {
            // Mark as failed
            for (auto& trans : available_translations) {
                if (trans.name == name) {
                    trans.is_downloading = false;
                    trans.download_progress = 0.0f;
                    break;
                }
            }
            std::cerr << "Failed to download " << name << ": " << e.what() << std::endl;
        }
    }).detach();
}

void VerseFinderApp::updateAvailableTranslationStatus() {
    const auto& loaded_translations = bible.getTranslations();
    
    for (auto& available : available_translations) {
        if (!available.is_downloading) {
            available.is_downloaded = false;
            for (const auto& loaded : loaded_translations) {
                if (loaded.name == available.name || loaded.abbreviation == available.abbreviation) {
                    available.is_downloaded = true;
                    break;
                }
            }
        }
    }
}

void VerseFinderApp::switchToTranslation(const std::string& translation_name) {
    const auto& translations = bible.getTranslations();
    for (const auto& trans : translations) {
        if (trans.abbreviation == translation_name || trans.name == translation_name) {
            current_translation = trans;
            
            // Add to recent translations
            userSettings.addToRecentTranslations(trans.abbreviation);
            
            // Re-perform search with new translation
            if (strlen(search_input) > 0) {
                performSearch();
            }
            break;
        }
    }
}

bool VerseFinderApp::isTranslationAvailable(const std::string& name) const {
    const auto& translations = bible.getTranslations();
    return std::any_of(translations.begin(), translations.end(),
                      [&name](const auto& trans) { 
                          return trans.name == name || trans.abbreviation == name; 
                      });
}

bool VerseFinderApp::saveSettings() const {
    try {
        std::string settingsPath = getSettingsFilePath();
        
        // Update window state in settings if remembering position
        if (userSettings.display.rememberWindowState && window) {
            int width, height, xpos, ypos;
            glfwGetWindowSize(window, &width, &height);
            glfwGetWindowPos(window, &xpos, &ypos);
            
            // Create a mutable copy to update window state
            UserSettings mutableSettings = userSettings;
            mutableSettings.display.windowWidth = width;
            mutableSettings.display.windowHeight = height;
            mutableSettings.display.windowPosX = xpos;
            mutableSettings.display.windowPosY = ypos;
            
            json settingsJson = mutableSettings.toJson();
            std::ofstream file(settingsPath);
            if (!file.is_open()) {
                std::cerr << "Failed to open settings file for writing: " << settingsPath << std::endl;
                return false;
            }
            
            file << settingsJson.dump(4);
            file.close();
        } else {
            json settingsJson = userSettings.toJson();
            std::ofstream file(settingsPath);
            if (!file.is_open()) {
                std::cerr << "Failed to open settings file for writing: " << settingsPath << std::endl;
                return false;
            }
            
            file << settingsJson.dump(4);
            file.close();
        }
        
        std::cout << "Settings saved to: " << settingsPath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to save settings: " << e.what() << std::endl;
        return false;
    }
}

bool VerseFinderApp::loadSettings() {
    try {
        std::string settingsPath = getSettingsFilePath();
        
        if (!std::filesystem::exists(settingsPath)) {
            std::cout << "Settings file not found, using defaults: " << settingsPath << std::endl;
            userSettings.applyDefaults();
            return saveSettings(); // Save default settings
        }
        
        std::ifstream file(settingsPath);
        if (!file.is_open()) {
            std::cerr << "Failed to open settings file: " << settingsPath << std::endl;
            userSettings.applyDefaults();
            return false;
        }
        
        json settingsJson;
        file >> settingsJson;
        file.close();
        
        userSettings.fromJson(settingsJson);
        
        if (!userSettings.validate()) {
            std::cerr << "Invalid settings detected, applying defaults" << std::endl;
            userSettings.applyDefaults();
            return saveSettings();
        }
        
        std::cout << "Settings loaded from: " << settingsPath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load settings: " << e.what() << std::endl;
        userSettings.applyDefaults();
        return saveSettings(); // Save default settings
    }
}

void VerseFinderApp::resetSettingsToDefault() {
    userSettings.applyDefaults();
    saveSettings();
}

bool VerseFinderApp::exportSettings(const std::string& filepath) const {
    try {
        json settingsJson = userSettings.toJson();
        std::ofstream file(filepath);
        if (!file.is_open()) {
            return false;
        }
        file << settingsJson.dump(4);
        file.close();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool VerseFinderApp::importSettings(const std::string& filepath) {
    try {
        if (!std::filesystem::exists(filepath)) {
            return false;
        }
        
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }
        
        json settingsJson;
        file >> settingsJson;
        file.close();
        
        UserSettings tempSettings;
        tempSettings.fromJson(settingsJson);
        
        if (!tempSettings.validate()) {
            return false;
        }
        
        userSettings = tempSettings;
        return saveSettings();
    } catch (const std::exception&) {
        return false;
    }
}

std::string VerseFinderApp::getTranslationFilename(const std::string& translation_name) const {
    // Map translation names to expected filenames
    std::unordered_map<std::string, std::string> name_to_filename = {
        {"King James Version", "King_James_Version.json"},
        {"New International Version", "New_International_Version.json"},
        {"English Standard Version", "English_Standard_Version.json"},
        {"New Living Translation", "New_Living_Translation.json"},
        {"American Standard Version", "American_Standard_Version.json"},
        {"World English Bible", "World_English_Bible.json"},
        {"New King James Version", "New_King_James_Version.json"},
        {"The Message", "The_Message.json"}
    };
    
    auto it = name_to_filename.find(translation_name);
    if (it != name_to_filename.end()) {
        return it->second;
    }
    
    // Fallback: convert name to filename format
    std::string filename = translation_name + ".json";
    std::replace(filename.begin(), filename.end(), ' ', '_');
    return filename;
}

void VerseFinderApp::scanForExistingTranslations() {
    // Common search paths for translation files
    std::vector<std::string> search_directories = {
        getExecutablePath() + "/translations/",
        getExecutablePath() + "/",
        getExecutablePath() + "/data/",
        "./translations/",
        "./"
    };
    
    for (auto& available : available_translations) {
        if (!available.is_downloading) {
            available.is_downloaded = false;
            
            std::string expected_filename = getTranslationFilename(available.name);
            
            // Check if file exists in any of the search directories
            for (const auto& dir : search_directories) {
                std::string full_path = dir + expected_filename;
                std::ifstream file(full_path);
                if (file.is_open()) {
                    // Read and load the translation
                    std::stringstream buffer;
                    buffer << file.rdbuf();
                    std::string content = buffer.str();
                    file.close();
                    
                    try {
                        // Validate JSON format before loading
                        json test_parse = json::parse(content);
                        if (test_parse.contains("translation") && test_parse.contains("books")) {
                            bible.addTranslation(content);
                            available.is_downloaded = true;
                            std::cout << "Loaded existing translation: " << available.name 
                                      << " from " << full_path << std::endl;
                        } else {
                            std::cout << "Invalid format for translation file: " << full_path << std::endl;
                        }
                    } catch (const json::exception& e) {
                        std::cout << "Failed to parse translation file " << full_path 
                                  << ": " << e.what() << std::endl;
                    }
                    break;
                }
            }
        }
    }
}

std::string VerseFinderApp::downloadFromUrl(const std::string& url) const {
    // Create a temporary file for the download
    std::string temp_file = "/tmp/bible_download_" + std::to_string(std::time(nullptr)) + ".json";
    
    // Use curl to download the file
    std::string curl_command = "curl -s -L -f \"" + url + "\" -o \"" + temp_file + "\"";
    
    std::cout << "Downloading from: " << url << std::endl;
    
    int result = system(curl_command.c_str());
    if (result != 0) {
        std::cerr << "Failed to download from URL: " << url << std::endl;
        return "";
    }
    
    // Read the downloaded file
    std::ifstream file(temp_file);
    if (!file.is_open()) {
        std::cerr << "Failed to open downloaded file: " << temp_file << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    // Clean up temporary file
    std::remove(temp_file.c_str());
    
    return content;
}

std::string VerseFinderApp::extractFilenameFromUrl(const std::string& url, const std::string& translation_name) const {
    // Map URL patterns to proper filenames
    std::unordered_map<std::string, std::string> url_to_filename = {
        {"kjv.json", "King_James_Version.json"},
        {"niv.json", "New_International_Version.json"},
        {"esv.json", "English_Standard_Version.json"},
        {"nlt.json", "New_Living_Translation.json"},
        {"asv.json", "American_Standard_Version.json"},
        {"web.json", "World_English_Bible.json"},
        {"nkjv.json", "New_King_James_Version.json"},
        {"msg.json", "The_Message.json"}
    };
    
    // Extract the last part of the URL (e.g., "niv.json" from "https://api.getbible.net/v2/niv.json")
    size_t last_slash = url.find_last_of('/');
    if (last_slash != std::string::npos && last_slash < url.length() - 1) {
        std::string url_filename = url.substr(last_slash + 1);
        
        auto it = url_to_filename.find(url_filename);
        if (it != url_to_filename.end()) {
            return it->second;
        }
    }
    
    // Fallback to using translation name
    return getTranslationFilename(translation_name);
}

// Presentation Mode Implementation
void VerseFinderApp::initPresentationWindow() {
    if (presentation_window) {
        return; // Already created
    }
    
    auto monitors = getAvailableMonitors();
    GLFWmonitor* target_monitor = nullptr;
    
    // Select monitor based on settings
    if (userSettings.presentation.monitorIndex < static_cast<int>(monitors.size())) {
        target_monitor = monitors[userSettings.presentation.monitorIndex];
    } else if (!monitors.empty()) {
        target_monitor = monitors[0]; // Fallback to primary monitor
    }
    
    if (!target_monitor) {
        std::cerr << "No monitor available for presentation window" << std::endl;
        return;
    }
    
    // Get monitor properties
    const GLFWvidmode* mode = glfwGetVideoMode(target_monitor);
    int monitor_x, monitor_y;
    glfwGetMonitorPos(target_monitor, &monitor_x, &monitor_y);
    
    // Set window hints for presentation window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    // Create window
    if (userSettings.presentation.fullscreen) {
        presentation_window = glfwCreateWindow(mode->width, mode->height, 
                                             userSettings.presentation.windowTitle.c_str(), 
                                             target_monitor, window);
    } else {
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); // Borderless for OBS
        presentation_window = glfwCreateWindow(userSettings.presentation.windowWidth, 
                                             userSettings.presentation.windowHeight,
                                             userSettings.presentation.windowTitle.c_str(), 
                                             nullptr, window);
        
        // Position on target monitor
        int window_x = monitor_x + (mode->width - userSettings.presentation.windowWidth) / 2;
        int window_y = monitor_y + (mode->height - userSettings.presentation.windowHeight) / 2;
        
        if (userSettings.presentation.windowPosX >= 0 && userSettings.presentation.windowPosY >= 0) {
            window_x = monitor_x + userSettings.presentation.windowPosX;
            window_y = monitor_y + userSettings.presentation.windowPosY;
        }
        
        glfwSetWindowPos(presentation_window, window_x, window_y);
    }
    
    if (!presentation_window) {
        std::cerr << "Failed to create presentation window" << std::endl;
        return;
    }
    
    // Hide cursor if enabled
    if (userSettings.presentation.autoHideCursor) {
        glfwSetInputMode(presentation_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }
    
    std::cout << "Presentation window created successfully" << std::endl;
}

void VerseFinderApp::destroyPresentationWindow() {
    if (presentation_window) {
        glfwDestroyWindow(presentation_window);
        presentation_window = nullptr;
        presentation_mode_active = false;
    }
}

void VerseFinderApp::renderPresentationWindow() {
    if (!presentation_window) {
        return;
    }
    
    // Make presentation window context current
    glfwMakeContextCurrent(presentation_window);
    
    int display_w, display_h;
    glfwGetFramebufferSize(presentation_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    
    // Parse background color
    float bg_r = 0.0f, bg_g = 0.0f, bg_b = 0.0f;
    if (userSettings.presentation.backgroundColor.length() == 7 && userSettings.presentation.backgroundColor[0] == '#') {
        std::string hex = userSettings.presentation.backgroundColor.substr(1);
        bg_r = std::stoi(hex.substr(0, 2), 0, 16) / 255.0f;
        bg_g = std::stoi(hex.substr(2, 2), 0, 16) / 255.0f;
        bg_b = std::stoi(hex.substr(4, 2), 0, 16) / 255.0f;
    }
    
    glClearColor(bg_r, bg_g, bg_b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Start ImGui frame for presentation window
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // Create fullscreen invisible window for rendering
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                           ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus |
                           ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoDecoration;
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(display_w, display_h));
    
    if (ImGui::Begin("PresentationDisplay", nullptr, flags)) {
        if (!presentation_blank_screen && !current_displayed_verse.empty()) {
            // Parse text color
            float text_r = 1.0f, text_g = 1.0f, text_b = 1.0f;
            if (userSettings.presentation.textColor.length() == 7 && userSettings.presentation.textColor[0] == '#') {
                std::string hex = userSettings.presentation.textColor.substr(1);
                text_r = std::stoi(hex.substr(0, 2), 0, 16) / 255.0f;
                text_g = std::stoi(hex.substr(2, 2), 0, 16) / 255.0f;
                text_b = std::stoi(hex.substr(4, 2), 0, 16) / 255.0f;
            }
            
            // Parse reference color
            float ref_r = 0.8f, ref_g = 0.8f, ref_b = 0.8f;
            if (userSettings.presentation.referenceColor.length() == 7 && userSettings.presentation.referenceColor[0] == '#') {
                std::string hex = userSettings.presentation.referenceColor.substr(1);
                ref_r = std::stoi(hex.substr(0, 2), 0, 16) / 255.0f;
                ref_g = std::stoi(hex.substr(2, 2), 0, 16) / 255.0f;
                ref_b = std::stoi(hex.substr(4, 2), 0, 16) / 255.0f;
            }
            
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(userSettings.presentation.textPadding, userSettings.presentation.textPadding));
            
            // Calculate text area
            float available_width = display_w - (2 * userSettings.presentation.textPadding);
            float available_height = display_h - (2 * userSettings.presentation.textPadding);
            
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Use default font, we'll scale it
            
            // Set large font size for presentation
            float font_scale = userSettings.presentation.fontSize / ImGui::GetFontSize();
            ImGui::SetWindowFontScale(font_scale);
            
            // Calculate text size and position for centering
            ImVec2 verse_size = ImGui::CalcTextSize(current_displayed_verse.c_str(), nullptr, false, available_width);
            ImVec2 ref_size = ImVec2(0, 0);
            if (userSettings.presentation.showReference && !current_displayed_reference.empty()) {
                ref_size = ImGui::CalcTextSize(current_displayed_reference.c_str());
            }
            
            float total_height = verse_size.y;
            if (userSettings.presentation.showReference) {
                total_height += ref_size.y + 20; // Add spacing
            }
            
            float start_y = (available_height - total_height) / 2;
            if (start_y < 0) start_y = 0;
            
            ImGui::SetCursorPos(ImVec2(userSettings.presentation.textPadding, userSettings.presentation.textPadding + start_y));
            
            // Display verse text
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(text_r, text_g, text_b, presentation_fade_alpha));
            
            if (userSettings.presentation.textAlignment == "center") {
                float text_width = ImGui::CalcTextSize(current_displayed_verse.c_str(), nullptr, false, available_width).x;
                if (text_width < available_width) {
                    ImGui::SetCursorPosX(userSettings.presentation.textPadding + (available_width - text_width) / 2);
                }
            } else if (userSettings.presentation.textAlignment == "right") {
                float text_width = ImGui::CalcTextSize(current_displayed_verse.c_str(), nullptr, false, available_width).x;
                if (text_width < available_width) {
                    ImGui::SetCursorPosX(userSettings.presentation.textPadding + available_width - text_width);
                }
            }
            
            ImGui::TextWrapped("%s", current_displayed_verse.c_str());
            ImGui::PopStyleColor();
            
            // Display reference if enabled
            if (userSettings.presentation.showReference && !current_displayed_reference.empty()) {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ref_r, ref_g, ref_b, presentation_fade_alpha));
                
                if (userSettings.presentation.textAlignment == "center") {
                    float ref_width = ImGui::CalcTextSize(current_displayed_reference.c_str()).x;
                    if (ref_width < available_width) {
                        ImGui::SetCursorPosX(userSettings.presentation.textPadding + (available_width - ref_width) / 2);
                    }
                } else if (userSettings.presentation.textAlignment == "right") {
                    float ref_width = ImGui::CalcTextSize(current_displayed_reference.c_str()).x;
                    if (ref_width < available_width) {
                        ImGui::SetCursorPosX(userSettings.presentation.textPadding + available_width - ref_width);
                    }
                }
                
                ImGui::Text("%s", current_displayed_reference.c_str());
                ImGui::PopStyleColor();
            }
            
            ImGui::PopFont();
            ImGui::PopStyleVar();
        }
    }
    ImGui::End();
    
    // Render
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    glfwSwapBuffers(presentation_window);
    
    // Switch back to main window context
    glfwMakeContextCurrent(window);
}

void VerseFinderApp::renderPresentationPreview() {
    if (!userSettings.presentation.enabled) {
        return;
    }
    
    ImGui::Separator();
    ImGui::Text("Presentation Preview");
    
    // Create a bordered child window for preview
    ImVec2 preview_size(400, 225); // 16:9 aspect ratio
    
    if (ImGui::BeginChild("PresentationPreview", preview_size, true)) {
        // Parse background color for preview
        float bg_r = 0.0f, bg_g = 0.0f, bg_b = 0.0f;
        if (userSettings.presentation.backgroundColor.length() == 7 && userSettings.presentation.backgroundColor[0] == '#') {
            std::string hex = userSettings.presentation.backgroundColor.substr(1);
            bg_r = std::stoi(hex.substr(0, 2), 0, 16) / 255.0f;
            bg_g = std::stoi(hex.substr(2, 2), 0, 16) / 255.0f;
            bg_b = std::stoi(hex.substr(4, 2), 0, 16) / 255.0f;
        }
        
        // Draw background
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImGui::GetContentRegionAvail();
        draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                                IM_COL32(bg_r * 255, bg_g * 255, bg_b * 255, 255));
        
        if (!presentation_blank_screen && !current_displayed_verse.empty()) {
            // Display preview text (smaller scale)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            
            float scale = 0.3f; // Scale down for preview
            ImGui::SetWindowFontScale(scale);
            
            // Center the text
            ImVec2 text_size = ImGui::CalcTextSize(current_displayed_verse.c_str(), nullptr, false, canvas_size.x);
            float start_y = (canvas_size.y - text_size.y) / 2;
            if (start_y < 0) start_y = 0;
            
            ImGui::SetCursorPos(ImVec2(10, start_y));
            ImGui::TextWrapped("%s", current_displayed_verse.c_str());
            
            if (userSettings.presentation.showReference && !current_displayed_reference.empty()) {
                ImGui::Text("%s", current_displayed_reference.c_str());
            }
            
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopStyleColor();
        } else if (presentation_blank_screen) {
            // Show "BLANK" indicator in preview
            ImGui::SetCursorPos(ImVec2(canvas_size.x / 2 - 30, canvas_size.y / 2 - 10));
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "BLANK");
        }
    }
    ImGui::EndChild();
    
    // Presentation controls
    ImGui::Spacing();
    
    if (!presentation_mode_active) {
        if (ImGui::Button("Start Presentation Mode")) {
            togglePresentationMode();
        }
    } else {
        if (ImGui::Button("Stop Presentation Mode")) {
            togglePresentationMode();
        }
        ImGui::SameLine();
        if (ImGui::Button(presentation_blank_screen ? "Show Display" : "Blank Screen")) {
            toggleBlankScreen();
        }
    }
}

void VerseFinderApp::togglePresentationMode() {
    if (!presentation_mode_active) {
        initPresentationWindow();
        if (presentation_window) {
            presentation_mode_active = true;
            userSettings.presentation.enabled = true;
        }
    } else {
        destroyPresentationWindow();
        presentation_mode_active = false;
    }
}

void VerseFinderApp::displayVerseOnPresentation(const std::string& verse_text, const std::string& reference) {
    current_displayed_verse = verse_text;
    current_displayed_reference = reference;
    presentation_blank_screen = false;
    presentation_fade_alpha = 1.0f;
}

void VerseFinderApp::clearPresentationDisplay() {
    current_displayed_verse.clear();
    current_displayed_reference.clear();
}

void VerseFinderApp::toggleBlankScreen() {
    presentation_blank_screen = !presentation_blank_screen;
}

bool VerseFinderApp::isPresentationWindowActive() const {
    return presentation_window != nullptr && presentation_mode_active;
}

void VerseFinderApp::updatePresentationMonitorPosition() {
    if (!presentation_window) {
        return;
    }
    
    // Recreate window with new position
    destroyPresentationWindow();
    initPresentationWindow();
}

std::vector<GLFWmonitor*> VerseFinderApp::getAvailableMonitors() const {
    std::vector<GLFWmonitor*> monitors;
    int count;
    GLFWmonitor** monitor_array = glfwGetMonitors(&count);
    
    for (int i = 0; i < count; i++) {
        monitors.push_back(monitor_array[i]);
    }
    
    return monitors;
}

void VerseFinderApp::cleanup() {
    // Cleanup presentation window first
    destroyPresentationWindow();
    
    if (window) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        glfwDestroyWindow(window);
        glfwTerminate();
        window = nullptr;
    }
}