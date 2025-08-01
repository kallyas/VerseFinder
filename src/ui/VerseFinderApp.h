#ifndef VERSEFINDERAPP_H
#define VERSEFINDERAPP_H

// OpenGL loader - must be included before GLFW
#ifdef IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <GL/glew.h>
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>
#elif defined(IMGUI_IMPL_OPENGL_LOADER_CUSTOM)
// Custom OpenGL loader - use system OpenGL
#ifdef __APPLE__
#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif
#endif

#include "../core/VerseFinder.h"
#include "../core/FuzzySearch.h"
#include "../core/UserSettings.h"
#include "../core/MemoryMonitor.h"
#include "../core/IncrementalSearch.h"
#include "../integrations/IntegrationManager.h"
#include "../service/ServicePlan.h"
#include "../api/ApiServer.h"
#include "components/SearchComponent.h"
#include "components/TranslationSelector.h"
#include "settings/ThemeManager.h"
#include "system/FontManager.h"
#include "system/WindowManager.h"
#include "system/PlatformUtils.h"
#include "modals/SettingsModal.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <chrono>

enum class UIScreen {
    SPLASH,
    MAIN,
    SETTINGS,
    SERVICE_PLANNING
};

struct AvailableTranslation {
    std::string name;
    std::string abbreviation;
    std::string url;
    std::string description;
    bool is_downloaded;
    bool is_downloading;
    float download_progress;
};

class VerseFinderApp {
private:
    // Core GLFW and OpenGL components
    GLFWwindow* window;
    GLFWwindow* presentation_window;
    
    // Presentation state
    bool presentation_mode_active = false;
    std::string current_displayed_verse;
    std::string current_displayed_reference;
    float presentation_fade_alpha = 1.0f;
    bool presentation_blank_screen = false;
    
    // User settings
    UserSettings userSettings;
    
    // Core application state
    VerseFinder bible;
    TranslationInfo current_translation;
    
    // Search state
    char search_input[512] = "";
    std::vector<std::string> search_results;
    int selected_result_index = -1;
    std::string selected_verse_text;
    std::string last_search_query;
    bool auto_search = true;
    
    // Fuzzy search state
    bool fuzzy_search_enabled = false;
    std::vector<std::string> query_suggestions;
    std::vector<FuzzyMatch> book_suggestions;
    
    // Chapter viewing state
    bool is_viewing_chapter = false;
    std::string current_chapter_book;
    int current_chapter_number = -1;
    
    // Performance monitoring
    double last_search_time_ms = 0.0;
    bool show_performance_stats = false;
    bool show_memory_monitor = false;
    std::unique_ptr<IncrementalSearch> incremental_search;
    std::vector<std::string> auto_complete_suggestions;
    bool show_auto_complete = false;
    
    // Splash screen state
    UIScreen current_screen = UIScreen::SPLASH;
    std::chrono::steady_clock::time_point app_start_time;
    
    // Service planning integration
    std::unique_ptr<IntegrationManager> integration_manager;
    std::unique_ptr<ServicePlan> current_service_plan;
    int selected_integration_type = 0;
    
    // API server
    std::unique_ptr<ApiServer> api_server;
    bool api_server_enabled = false;
    float splash_progress = 0.0f;
    std::string splash_status = "Initializing...";
    
    // UI Components
    std::unique_ptr<SearchComponent> search_component;
    std::unique_ptr<TranslationSelector> translation_selector;
    std::unique_ptr<ThemeManager> theme_manager;
    std::unique_ptr<FontManager> font_manager;
    std::unique_ptr<WindowManager> window_manager;
    std::unique_ptr<SettingsModal> settings_modal;
    
    // UI state
    bool show_verse_modal = false;
    bool show_settings_window = false;
    bool show_about_window = false;
    bool show_help_window = false;
    bool show_performance_window = false;
    bool show_memory_window = false;
    bool show_integrations_window = false;
    
    // API server setup
    void setupApiRoutes();
    
    // Available translations for download
    std::vector<AvailableTranslation> available_translations = {
        {"King James Version", "KJV", "https://api.getbible.net/v2/kjv.json", 
         "The classic English translation from 1611", false, false, 0.0f},
        {"New International Version", "NIV", "https://api.getbible.net/v2/niv.json", 
         "Modern English translation, widely used", false, false, 0.0f},
        {"English Standard Version", "ESV", "https://api.getbible.net/v2/esv.json", 
         "Literal yet readable modern translation", false, false, 0.0f},
        {"New Living Translation", "NLT", "https://api.getbible.net/v2/nlt.json", 
         "Thought-for-thought contemporary translation", false, false, 0.0f},
        {"American Standard Version", "ASV", "https://api.getbible.net/v2/asv.json", 
         "Classic American revision of the KJV", false, false, 0.0f},
        {"World English Bible", "WEB", "https://api.getbible.net/v2/web.json", 
         "Modern public domain translation", false, false, 0.0f},
        { "New King James Version", "NKJV", "https://api.getbible.net/v2/nkjv.json",
         "Modern update of the KJV with updated language", false, false, 0.0f },
        {"The Message", "MSG", "https://api.getbible.net/v2/msg.json",
         "Paraphrase translation for contemporary readers", false, false, 0.0f }
    };
    
    // UI rendering methods
    void renderMainWindow();
    void renderSplashScreen();
    void renderSearchArea();
    void renderSearchResults();
    void renderTranslationSelector();
    void renderStatusBar();
    void renderAutoComplete();
    
    // Modal and window rendering
    void renderVerseModal();
    void renderSettingsWindow();
    void renderServicePlanningScreen();
    void renderIntegrationsWindow();
    void renderAboutWindow();
    void renderHelpWindow();
    void renderPerformanceWindow();
    void renderMemoryWindow();
    
    // Presentation mode methods
    void initPresentationWindow();
    void destroyPresentationWindow();
    void renderPresentationWindow();
    void renderPresentationPreview();
    void togglePresentationMode();
    void displayVerseOnPresentation(const std::string& verse_text, const std::string& reference);
    void clearPresentationDisplay();
    void toggleBlankScreen();
    bool isPresentationWindowActive() const;
    void updatePresentationMonitorPosition();
    std::vector<GLFWmonitor*> getAvailableMonitors() const;
    
    // Utility methods
    void performSearch();
    void performIncrementalSearch();
    void updateAutoComplete();
    void clearSearch();
    void selectResult(int index);
    void copyToClipboard(const std::string& text);
    std::string formatVerseReference(const std::string& verse_text);
    std::string formatVerseText(const std::string& verse_text);
    void navigateToVerse(int direction);  // Navigate to next/previous verse
    void jumpToVerse(const std::string& book, int chapter, int verse);  // Jump to specific verse
    
    // Translation management
    void downloadTranslation(const std::string& url, const std::string& name);
    void updateAvailableTranslationStatus();
    void switchToTranslation(const std::string& translation_name);
    bool isTranslationAvailable(const std::string& name) const;
    std::string getTranslationFilename(const std::string& translation_name) const;
    void scanForExistingTranslations();
    
    // HTTP download utilities
    std::string downloadFromUrl(const std::string& url) const;
    std::string extractFilenameFromUrl(const std::string& url, const std::string& translation_name) const;
    
    // Event handling
    static void glfwErrorCallback(int error, const char* description);
    void handleKeyboardShortcuts();
    
    // File operations
    bool saveSettings() const;
    bool loadSettings();
    void resetSettingsToDefault();
    bool exportSettings(const std::string& filepath) const;
    bool importSettings(const std::string& filepath);

public:
    VerseFinderApp();
    ~VerseFinderApp();
    
    bool init();
    void run();
    void cleanup();
};

#endif //VERSEFINDERAPP_H