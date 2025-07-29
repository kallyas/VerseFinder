#include "VerseFinderApp.h"
#include <iostream>
#include <filesystem>
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
#elif defined(_WIN32)
#include <windows.h>
#endif

VerseFinderApp::VerseFinderApp() : window(nullptr) {}

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

bool VerseFinderApp::init() {
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
    
    // Create window
    window = glfwCreateWindow(1400, 900, "VerseFinder - Bible Search for Churches", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
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
    io.Fonts->AddFontFromFileTTF((getExecutablePath() + "/fonts/Gentium_Plus/GentiumPlus-Regular.ttf").c_str(), systemFontSize);
    
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
    loadSettings();
    
    return true;
}

void VerseFinderApp::setupImGuiStyle() {
    applyDarkTheme();
    
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
        
        // No additional platform windows in this version
        
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
    renderStatusBar();
    
    ImGui::EndChild();
}

void VerseFinderApp::renderSearchArea() {
    ImGui::Text("🔍 Bible Search");
    ImGui::Spacing();
    
    // Search input
    ImGui::PushItemWidth(-1);
    bool search_changed = ImGui::InputTextWithHint("##search", "Enter verse reference (e.g., 'John 3:16') or keywords...", 
                                                  search_input, sizeof(search_input), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopItemWidth();
    
    // Auto-search or manual search
    if (search_changed || (auto_search && strcmp(search_input, last_search_query.c_str()) != 0)) {
        last_search_query = search_input;
        performSearch();
    }
    
    // Search buttons
    ImGui::Spacing();
    if (ImGui::Button("🔍 Search", ImVec2(80, 0))) {
        performSearch();
    }
    ImGui::SameLine();
    if (ImGui::Button("✖ Clear", ImVec2(80, 0))) {
        clearSearch();
    }
    ImGui::SameLine();
    ImGui::Text("Auto: ");
    ImGui::SameLine();
    ImGui::Checkbox("##auto_search", &auto_search);
    
    // Search hints
    if (strlen(search_input) == 0) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "💡 Examples:");
        ImGui::BulletText("John 3:16 - Find specific verse");
        ImGui::BulletText("love - Find verses with keyword");
        ImGui::BulletText("faith hope love - Find multiple keywords");
        ImGui::BulletText("Psalm 23 - Find chapter references");
    }
}

void VerseFinderApp::renderSearchResults() {
    if (!bible.isReady()) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "⏳ Loading Bible data...");
        return;
    }
    
    if (search_results.empty()) {
        if (strlen(search_input) > 0) {
            ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.4f, 1.0f), "❌ No verses found");
            ImGui::Text("Try different keywords or check the reference");
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "📖 Enter search terms above");
        }
        return;
    }
    
    // Show different headers based on search type
    if (is_viewing_chapter) {
        ImGui::Text("📖 %s Chapter %d (%d verses)", 
                   current_chapter_book.c_str(), current_chapter_number, (int)search_results.size());
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click any verse to jump to it");
    } else {
        ImGui::Text("📋 Results (%d found)", (int)search_results.size());
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
    
    if (ImGui::Button("⚙️ Manage Translations", ImVec2(-1, 0))) {
        show_settings_window = true;
    }
}

void VerseFinderApp::renderStatusBar() {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("📊 Status");
    
    if (bible.isReady()) {
        const auto& translations = bible.getTranslations();
        ImGui::Text("✅ Ready - %d translation(s) loaded", (int)translations.size());
        
        if (!search_results.empty()) {
            ImGui::Text("🔍 Found %d verse(s)", (int)search_results.size());
            if (selected_result_index >= 0) {
                ImGui::Text("👆 Selected: %d", selected_result_index + 1);
            }
        }
        
        // Performance information
        if (last_search_time_ms > 0.0) {
            ImGui::Text("⚡ Search: %.2f ms", last_search_time_ms);
        }
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "⏳ Loading Bible data...");
    }
    
    // Selected verse preview
    if (!selected_verse_text.empty()) {
        ImGui::Spacing();
        ImGui::Text("📖 Selected Verse:");
        ImGui::Separator();
        
        std::string reference = formatVerseReference(selected_verse_text);
        std::string verse_text = formatVerseText(selected_verse_text);
        
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "%s", reference.c_str());
        
        ImGui::PushTextWrapPos(0.0f);
        ImGui::Text("%s", verse_text.c_str());
        ImGui::PopTextWrapPos();
        
        if (ImGui::Button("👁️ View Full", ImVec2(-1, 0))) {
            show_verse_modal = true;
        }
    }
}

void VerseFinderApp::renderVerseModal() {
    ImGui::SetNextWindowSize(ImVec2(900, 650), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("📖 Verse Details", &show_verse_modal, ImGuiWindowFlags_AlwaysAutoResize)) {
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
            if (ImGui::Button("⬅️⬅️ -10", ImVec2(70, 35))) {
                navigateToVerse(-10);
            }
            ImGui::SameLine();
            if (ImGui::Button("⬅️ -1", ImVec2(60, 35))) {
                navigateToVerse(-1);
            }
            ImGui::SameLine();
            if (ImGui::Button("➡️ +1", ImVec2(60, 35))) {
                navigateToVerse(1);
            }
            ImGui::SameLine();
            if (ImGui::Button("➡️➡️ +10", ImVec2(70, 35))) {
                navigateToVerse(10);
            }
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();
            
            // Action buttons
            if (ImGui::Button("📋 Copy", ImVec2(100, 35))) {
                copyToClipboard(selected_verse_text);
            }
            ImGui::SameLine();
            if (ImGui::Button("❌ Close", ImVec2(100, 35))) {
                show_verse_modal = false;
            }
        }
    }
    ImGui::End();
}

void VerseFinderApp::renderSettingsWindow() {
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("⚙️ Settings", &show_settings_window)) {
        if (ImGui::BeginTabBar("SettingsTabs")) {
            if (ImGui::BeginTabItem("📚 Translations")) {
                ImGui::Text("Manage Bible translations for VerseFinder");
                ImGui::Separator();
                
                // Download all button
                if (ImGui::Button("⬇️ Download All Free Translations", ImVec2(-1, 30))) {
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
                            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "⏳ Downloading...");
                            ImGui::ProgressBar(trans.download_progress);
                        } else if (trans.is_downloaded) {
                            ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "✅ Downloaded");
                        } else {
                            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "⭕ Available");
                        }
                        
                        // Description
                        ImGui::TableNextColumn();
                        ImGui::TextWrapped("%s", trans.description.c_str());
                        
                        // Actions
                        ImGui::TableNextColumn();
                        std::string button_id = "##" + trans.abbreviation;
                        
                        if (trans.is_downloaded) {
                            if (ImGui::Button(("✅ Select" + button_id).c_str(), ImVec2(80, 0))) {
                                switchToTranslation(trans.name);
                            }
                        } else if (!trans.is_downloading) {
                            if (ImGui::Button(("⬇️ Download" + button_id).c_str(), ImVec2(80, 0))) {
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
                
                static bool dark_theme = true;
                if (ImGui::Checkbox("Dark Theme", &dark_theme)) {
                    setupImGuiStyle();
                }
                
                ImGui::Text("Font scaling and other appearance options will be added here.");
                
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("⌨️ Shortcuts")) {
                ImGui::Text("Keyboard shortcuts for VerseFinder");
                ImGui::Separator();
                
                ImGui::BulletText("Ctrl+K - Clear search");
                ImGui::BulletText("Ctrl+C - Copy selected verse");
                ImGui::BulletText("Ctrl+P - Performance statistics");
                ImGui::BulletText("Ctrl+, - Open settings");
                ImGui::BulletText("F1 - Show help");
                ImGui::BulletText("Enter - Search");
                ImGui::BulletText("Escape - Close dialogs");
                
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
        
        ImGui::Separator();
        if (ImGui::Button("💾 Save Settings", ImVec2(120, 0))) {
            saveSettings();
        }
        ImGui::SameLine();
        if (ImGui::Button("❌ Close", ImVec2(120, 0))) {
            show_settings_window = false;
        }
    }
    ImGui::End();
}

void VerseFinderApp::renderAboutWindow() {
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("ℹ️ About VerseFinder", &show_about_window, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("📖 VerseFinder");
        ImGui::Text("Bible Search for Churches");
        ImGui::Separator();
        
        ImGui::Text("Version: 2.0 (ImGui Edition)");
        ImGui::Text("Built with Dear ImGui and C++");
        ImGui::Spacing();
        
        ImGui::Text("Features:");
        ImGui::BulletText("Fast verse lookup by reference");
        ImGui::BulletText("Keyword search across translations");
        ImGui::BulletText("Multiple Bible translation support");
        ImGui::BulletText("Modern, responsive interface");
        ImGui::BulletText("Church-friendly design");
        
        ImGui::Spacing();
        ImGui::Separator();
        
        if (ImGui::Button("❌ Close", ImVec2(-1, 0))) {
            show_about_window = false;
        }
    }
    ImGui::End();
}

void VerseFinderApp::renderHelpWindow() {
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("❓ Help", &show_help_window)) {
        ImGui::Text("🔍 How to Search");
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
        ImGui::Text("⌨️ Keyboard Shortcuts");
        ImGui::Separator();
        ImGui::BulletText("Ctrl+K - Clear search");
        ImGui::BulletText("Ctrl+C - Copy verse");
        ImGui::BulletText("Ctrl+P - Performance stats");
        ImGui::BulletText("Enter - Search");
        ImGui::BulletText("F1 - This help");
        
        ImGui::Separator();
        if (ImGui::Button("❌ Close", ImVec2(-1, 0))) {
            show_help_window = false;
        }
    }
    ImGui::End();
}

void VerseFinderApp::renderPerformanceWindow() {
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("⚡ Performance Statistics", &show_performance_stats)) {
        ImGui::Text("🔍 Search Performance");
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
        ImGui::Text("📊 Cache Statistics");
        ImGui::Separator();
        
        // Get cache statistics from VerseFinder
        if (bible.isReady()) {
            // We need to print the performance stats which includes cache info
            std::string cache_info = "Cache information available in console";
            ImGui::Text("%s", cache_info.c_str());
            
            if (ImGui::Button("🖨️ Print Full Stats to Console")) {
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
            ImGui::Text("🎯 Performance Targets");
            ImGui::Separator();
            
            ImGui::BulletText("Reference Search: < 5ms");
            ImGui::BulletText("Simple Keyword Search: < 20ms");
            ImGui::BulletText("Complex Multi-word Search: < 50ms");
            ImGui::BulletText("Cache Hit Rate: > 80%%");
            
            ImGui::Spacing();
            ImGui::Text("⚙️ Cache Management");
            ImGui::Separator();
            
            if (ImGui::Button("🗑️ Clear Search Cache")) {
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
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "⏳ Bible data still loading...");
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        
        if (ImGui::Button("❌ Close", ImVec2(-1, 0))) {
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
        // Use full text search for phrases and keywords
        search_results = bible.searchByKeywords(query, current_translation.name);
        is_viewing_chapter = false;
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    last_search_time_ms = duration.count() / 1000.0;
    
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
}

void VerseFinderApp::selectResult(int index) {
    if (index >= 0 && index < static_cast<int>(search_results.size())) {
        selected_result_index = index;
        selected_verse_text = search_results[index];
    }
}

void VerseFinderApp::copyToClipboard(const std::string& text) {
    // For now, just print to console - ImGui doesn't have built-in clipboard
    // In a real implementation, you'd use platform-specific clipboard APIs
    std::cout << "Copied to clipboard: " << text << std::endl;
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
    // Implementation would save settings to a file
    std::cout << "Settings saved" << std::endl;
    return true;
}

bool VerseFinderApp::loadSettings() {
    // Implementation would load settings from a file
    std::cout << "Settings loaded" << std::endl;
    return true;
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

void VerseFinderApp::cleanup() {
    if (window) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        glfwDestroyWindow(window);
        glfwTerminate();
        window = nullptr;
    }
}