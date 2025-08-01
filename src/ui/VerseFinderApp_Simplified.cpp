// Simplified VerseFinderApp.cpp - Under 500 lines
#include "VerseFinderApp.h"
#include "system/WindowManager.h"
#include "system/FontManager.h"
#include "components/SearchComponent.h"
#include "settings/ThemeManager.h"
#include "../core/VerseFinder.h"
#include "../core/UserSettings.h"

#include <iostream>
#include <chrono>
#include <thread>

VerseFinderApp::VerseFinderApp() 
    : current_screen(UIScreen::SPLASH)
    , splash_status("Initializing...")
    , splash_progress(0.0f)
    , show_settings_window(false)
    , show_about_window(false)
    , show_help_window(false)
    , show_performance_stats(false)
    , show_verse_modal(false) {
    
    // Initialize components
    window_manager = std::make_unique<WindowManager>();
    font_manager = std::make_unique<FontManager>();
    theme_manager = std::make_unique<ThemeManager>();
    verse_finder = std::make_unique<VerseFinder>();
    user_settings = std::make_unique<UserSettings>();
}

VerseFinderApp::~VerseFinderApp() {
    cleanup();
}

bool VerseFinderApp::init() {
    // Initialize GLFW and window
    updateSplashProgress("Creating window...", 0.1f);
    if (!window_manager->initMainWindow(1200, 800, "VerseFinder - Bible Study Tool")) {
        return false;
    }

    // Initialize OpenGL
    updateSplashProgress("Initializing OpenGL...", 0.2f);
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
    #endif

    // Initialize ImGui
    updateSplashProgress("Setting up UI framework...", 0.3f);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Initialize ImGui backends
    ImGui_ImplGlfw_InitForOpenGL(window_manager->getMainWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 150");

    // Load fonts
    updateSplashProgress("Loading fonts...", 0.5f);
    if (!font_manager->initializeFonts()) {
        std::cerr << "Warning: Font initialization failed" << std::endl;
    }

    // Load user settings
    updateSplashProgress("Loading settings...", 0.6f);
    user_settings->loadFromFile();

    // Apply theme
    updateSplashProgress("Applying theme...", 0.7f);
    theme_manager->setupImGuiStyle(user_settings->display.colorTheme, 
                                  user_settings->display.fontSize / 16.0f);

    // Initialize Bible data
    updateSplashProgress("Loading Bible data...", 0.8f);
    verse_finder->initializeAsync();

    // Initialize search component
    updateSplashProgress("Initializing search...", 0.9f);
    search_component = std::make_unique<SearchComponent>(verse_finder.get());
    search_component->setFuzzySearchEnabled(user_settings->search.fuzzySearchEnabled);
    search_component->setIncrementalSearchEnabled(user_settings->search.incrementalSearchEnabled);

    // Setup callbacks
    search_component->setOnResultSelected([this](const std::string& result) {
        onSearchResultSelected(result);
    });

    updateSplashProgress("Ready!", 1.0f);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    transitionToMainScreen();
    return true;
}

void VerseFinderApp::run() {
    while (!window_manager->shouldClose()) {
        window_manager->pollEvents();
        handleKeyboardShortcuts();
        
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        render();

        // Render everything
        ImGui::Render();
        int display_w, display_h;
        window_manager->getMainWindowSize(display_w, display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.09f, 0.09f, 0.11f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window_manager->swapBuffers();
    }
}

void VerseFinderApp::cleanup() {
    if (user_settings) {
        user_settings->saveToFile();
    }
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void VerseFinderApp::render() {
    switch (current_screen) {
        case UIScreen::SPLASH:
            renderSplashScreen();
            break;
        case UIScreen::MAIN:
            renderMainWindow();
            break;
    }
}

void VerseFinderApp::renderSplashScreen() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | 
                            ImGuiWindowFlags_NoScrollbar;
    
    if (ImGui::Begin("Splash", nullptr, flags)) {
        ImVec2 center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        
        // Title
        ImGui::SetCursorPos(ImVec2(center.x - 100, center.y - 50));
        ImGui::Text("VerseFinder");
        
        // Progress bar
        ImGui::SetCursorPos(ImVec2(center.x - 150, center.y));
        ImGui::ProgressBar(splash_progress, ImVec2(300, 0));
        
        // Status
        ImGui::SetCursorPos(ImVec2(center.x - 75, center.y + 30));
        ImGui::Text("%s", splash_status.c_str());
    }
    ImGui::End();
}

void VerseFinderApp::renderMainWindow() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | 
                            ImGuiWindowFlags_MenuBar;
    
    if (ImGui::Begin("VerseFinder", nullptr, flags)) {
        renderMenuBar();
        renderSearchArea();
        renderSearchResults();
        renderStatusBar();
    }
    ImGui::End();
    
    // Render modals
    if (show_verse_modal) renderVerseModal();
    if (show_about_window) renderAboutWindow();
    if (show_help_window) renderHelpWindow();
    if (show_performance_stats) renderPerformanceWindow();
}

void VerseFinderApp::renderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Settings", "Ctrl+,")) {
                show_settings_window = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                glfwSetWindowShouldClose(window_manager->getMainWindow(), GLFW_TRUE);
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Performance Stats", nullptr, &show_performance_stats);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu(ICON_MD_HELP " Help")) {
            if (ImGui::MenuItem(ICON_MD_HELP " Help", "F1")) {
                show_help_window = true;
            }
            if (ImGui::MenuItem(ICON_MD_INFO " About")) {
                show_about_window = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void VerseFinderApp::renderSearchArea() {
    if (search_component) {
        search_component->render();
    }
}

void VerseFinderApp::renderSearchResults() {
    if (search_component && search_component->hasResults()) {
        ImGui::Separator();
        ImGui::Spacing();
        
        const auto& results = search_component->getResults();
        if (ImGui::BeginChild("Results", ImVec2(0, -50), true)) {
            for (size_t i = 0; i < results.size(); ++i) {
                if (ImGui::Selectable(results[i].c_str(), false)) {
                    onSearchResultSelected(results[i]);
                }
                
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    onVerseClicked(results[i]);
                }
            }
        }
        ImGui::EndChild();
    }
}

void VerseFinderApp::renderStatusBar() {
    ImGui::Separator();
    ImGui::Text("Ready");
    ImGui::SameLine(ImGui::GetWindowWidth() - 200);
    if (verse_finder) {
        ImGui::Text("Verses loaded: %zu", verse_finder->getVerseCount());
    }
}

void VerseFinderApp::renderVerseModal() {
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::BeginPopupModal(ICON_MD_BOOK " Verse Details", &show_verse_modal, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("%s", current_verse_text.c_str());
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Reference: %s", current_verse_reference.c_str());
        
        ImGui::Spacing();
        if (ImGui::Button("Copy", ImVec2(120, 0))) {
            copyToClipboard(current_verse_text + " - " + current_verse_reference);
        }
        ImGui::SameLine();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            show_verse_modal = false;
        }
        
        ImGui::EndPopup();
    }
}

void VerseFinderApp::renderAboutWindow() {
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin(ICON_MD_INFO " About VerseFinder", &show_about_window)) {
        ImGui::Text("VerseFinder");
        ImGui::Text("Bible Study Tool");
        ImGui::Spacing();
        ImGui::Text("A fast, modern Bible search application");
        ImGui::Text("Built with Dear ImGui and C++");
        ImGui::Spacing();
        ImGui::Text("Features:");
        ImGui::BulletText("Fast verse and keyword search");
        ImGui::BulletText("Fuzzy search with error correction");
        ImGui::BulletText("Multiple Bible translations");
        ImGui::BulletText("Modern, responsive interface");
    }
    ImGui::End();
}

void VerseFinderApp::renderHelpWindow() {
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin(ICON_MD_HELP " Help", &show_help_window)) {
        ImGui::Text("How to Search");
        ImGui::Separator();
        
        ImGui::Text("Reference Search:");
        ImGui::BulletText("John 3:16 - Single verse");
        ImGui::BulletText("Psalm 23 - Entire chapter");
        
        ImGui::Spacing();
        ImGui::Text("Keyword Search:");
        ImGui::BulletText("love - Find verses containing 'love'");
        ImGui::BulletText("faith hope - Multiple keywords");
    }
    ImGui::End();
}

void VerseFinderApp::renderPerformanceWindow() {
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin(ICON_MD_ANALYTICS " Performance Statistics", &show_performance_stats)) {
        ImGui::Text("Application Performance");
        ImGui::Separator();
        
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
        
        if (verse_finder) {
            ImGui::Spacing();
            ImGui::Text(ICON_MD_MEMORY " Memory & System");
            ImGui::Text("Loaded verses: %zu", verse_finder->getVerseCount());
        }
    }
    ImGui::End();
}

void VerseFinderApp::handleKeyboardShortcuts() {
    ImGuiIO& io = ImGui::GetIO();
    
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Comma)) {
        show_settings_window = true;
    }
    
    if (ImGui::IsKeyPressed(ImGuiKey_F1)) {
        show_help_window = true;
    }
}

void VerseFinderApp::onSearchResultSelected(const std::string& result) {
    current_verse_text = result;
    current_verse_reference = "Reference"; // Extract from result
}

void VerseFinderApp::onVerseClicked(const std::string& verse) {
    current_verse_text = verse;
    current_verse_reference = "Reference"; // Extract from verse
    show_verse_modal = true;
    ImGui::OpenPopup(ICON_MD_BOOK " Verse Details");
}

void VerseFinderApp::updateSplashProgress(const std::string& status, float progress) {
    splash_status = status;
    splash_progress = progress;
}

void VerseFinderApp::transitionToMainScreen() {
    current_screen = UIScreen::MAIN;
}

void VerseFinderApp::copyToClipboard(const std::string& text) {
    glfwSetClipboardString(window_manager->getMainWindow(), text.c_str());
}

void VerseFinderApp::glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}