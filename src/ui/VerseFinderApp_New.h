#ifndef VERSEFINDERAPP_H
#define VERSEFINDERAPP_H

// Forward declarations to reduce includes
class VerseFinder;
class UserSettings;
class WindowManager;
class FontManager;
class SearchComponent;
class ThemeManager;
class SettingsModal;

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <string>
#include <vector>
#include <memory>

enum class UIScreen {
    SPLASH,
    MAIN
};

class VerseFinderApp {
public:
    VerseFinderApp();
    ~VerseFinderApp();
    
    bool init();
    void run();
    void cleanup();

private:
    // Core components
    std::unique_ptr<VerseFinder> verse_finder;
    std::unique_ptr<UserSettings> user_settings;
    std::unique_ptr<WindowManager> window_manager;
    std::unique_ptr<FontManager> font_manager;
    std::unique_ptr<SearchComponent> search_component;
    std::unique_ptr<ThemeManager> theme_manager;
    std::unique_ptr<SettingsModal> settings_modal;
    
    // Application state
    UIScreen current_screen;
    std::string splash_status;
    float splash_progress;
    
    // UI state
    bool show_settings_window;
    bool show_about_window;
    bool show_help_window;
    bool show_performance_stats;
    bool show_verse_modal;
    
    // Current verse display
    std::string current_verse_text;
    std::string current_verse_reference;
    
    // Main rendering methods
    void render();
    void renderSplashScreen();
    void renderMainWindow();
    void renderMenuBar();
    void renderSearchArea();
    void renderSearchResults();
    void renderStatusBar();
    
    // Modal windows
    void renderVerseModal();
    void renderAboutWindow();
    void renderHelpWindow();
    void renderPerformanceWindow();
    
    // Event handling
    void handleKeyboardShortcuts();
    static void glfwErrorCallback(int error, const char* description);
    
    // Utility methods
    void initializeSplashScreen();
    void updateSplashProgress(const std::string& status, float progress);
    void transitionToMainScreen();
    void copyToClipboard(const std::string& text);
    
    // Callbacks for components
    void onSearchResultSelected(const std::string& result);
    void onVerseClicked(const std::string& verse);
};

#endif // VERSEFINDERAPP_H