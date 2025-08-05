#include "PresentationWindow.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <algorithm>

PresentationWindow::PresentationWindow(UserSettings& settings)
    : userSettings(settings), presentation_window(nullptr), presentation_mode_active(false),
      presentation_fade_alpha(1.0f), presentation_blank_screen(false) {
    
    // Initialize effects systems
    presentation_effects.loadPreset("default");
    media_manager.scanDirectory("media", true);
    media_manager.scanDirectory("backgrounds", true);
}

PresentationWindow::~PresentationWindow() {
    destroyPresentationWindow();
}

bool PresentationWindow::initPresentationWindow(GLFWwindow* main_window) {
    if (presentation_window) {
        std::cerr << "Presentation window already exists!" << std::endl;
        return false;
    }
    
    // Get monitor configuration
    int monitor_count;
    GLFWmonitor** monitors = glfwGetMonitors(&monitor_count);
    
    if (monitor_count < 2) {
        std::cerr << "Only one monitor detected. Presentation mode requires a second monitor." << std::endl;
        return false;
    }
    
    // Use the second monitor for presentation (index 1)
    GLFWmonitor* presentation_monitor = monitors[std::min(1, monitor_count - 1)];
    const GLFWvidmode* mode = glfwGetVideoMode(presentation_monitor);
    
    // Create fullscreen window on presentation monitor
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    
    presentation_window = glfwCreateWindow(
        mode->width, mode->height, 
        "VerseFinder Presentation", 
        presentation_monitor, nullptr
    );
    
    if (!presentation_window) {
        std::cerr << "Failed to create presentation window" << std::endl;
        return false;
    }
    
    // Position window on the monitor
    int monitor_x, monitor_y;
    glfwGetMonitorPos(presentation_monitor, &monitor_x, &monitor_y);
    glfwSetWindowPos(presentation_window, monitor_x, monitor_y);
    
    // Make the presentation window context current temporarily to set up rendering
    GLFWwindow* previous_context = glfwGetCurrentContext();
    glfwMakeContextCurrent(presentation_window);
    
    // Setup ImGui for presentation window
    ImGui_ImplGlfw_InitForOpenGL(presentation_window, false);
    
    // Restore main window context
    glfwMakeContextCurrent(previous_context);
    
    presentation_mode_active = true;
    std::cout << "Presentation window initialized on monitor " << 1 << std::endl;
    
    return true;
}

void PresentationWindow::destroyPresentationWindow() {
    if (presentation_window) {
        glfwDestroyWindow(presentation_window);
        presentation_window = nullptr;
        presentation_mode_active = false;
        std::cout << "Presentation window destroyed" << std::endl;
    }
}

void PresentationWindow::renderPresentationWindow() {
    if (!presentation_window) return;
    
    // Update animation system
    animation_system.update();
    
    // Switch to presentation window context
    GLFWwindow* previous_context = glfwGetCurrentContext();
    glfwMakeContextCurrent(presentation_window);
    
    // Clear and setup for rendering
    int display_w, display_h;
    glfwGetFramebufferSize(presentation_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    
    // Clear with transparent background first
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    setupPresentationStyle();
    
    // Render background
    renderBackground();
    
    if (!presentation_blank_screen && !current_displayed_verse.empty()) {
        renderEnhancedPresentationContent();
    }
    
    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    glfwSwapBuffers(presentation_window);
    
    // Restore main window context
    glfwMakeContextCurrent(previous_context);
}

void PresentationWindow::renderPresentationContent() {
    // Create fullscreen window for content
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | 
                                   ImGuiWindowFlags_NoMove | 
                                   ImGuiWindowFlags_NoResize | 
                                   ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoBackground;
    
    if (ImGui::Begin("Presentation Content", nullptr, window_flags)) {
        // Calculate text positioning
        float window_width = io.DisplaySize.x;
        float window_height = io.DisplaySize.y;
        
        // Calculate text size with wrapping
        float wrap_width = window_width * 0.9f; // 90% of window width
        calculateTextLayout(current_displayed_verse, wrap_width);
        
        ImVec2 text_size = ImGui::CalcTextSize(current_displayed_verse.c_str(), nullptr, false, wrap_width);
        ImVec2 ref_size = ImGui::CalcTextSize(current_displayed_reference.c_str());
        
        // Total content height
        float total_height = text_size.y + ref_size.y + 40; // 40px spacing
        
        // Center vertically
        float start_y = (window_height - total_height) * 0.5f;
        
        // Set cursor position for verse text
        ImGui::SetCursorPos(ImVec2((window_width - wrap_width) * 0.5f, start_y));
        
        // Apply text color and alpha
        ImVec4 text_color = hexToImVec4(userSettings.presentation.textColor);
        text_color.w *= presentation_fade_alpha;
        ImGui::PushStyleColor(ImGuiCol_Text, text_color);
        
        // Render verse text with wrapping
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrap_width);
        ImGui::TextWrapped("%s", current_displayed_verse.c_str());
        ImGui::PopTextWrapPos();
        
        // Add spacing between verse and reference
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
        
        // Center and render reference
        float ref_x = (window_width - ref_size.x) * 0.5f;
        ImGui::SetCursorPosX(ref_x);
        ImGui::Text("%s", current_displayed_reference.c_str());
        
        ImGui::PopStyleColor();
    }
    ImGui::End();
}

void PresentationWindow::renderPresentationPreview() {
    if (!presentation_window) return;
    
    ImGui::Text("Presentation Preview");
    ImGui::Separator();
    
    if (current_displayed_verse.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No verse displayed");
    } else {
        // Show preview with background color
        ImVec4 bg_color = hexToImVec4(userSettings.presentation.backgroundColor);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_color);
        
        if (ImGui::BeginChild("Preview", ImVec2(0, 150), true)) {
            if (!presentation_blank_screen) {
                ImVec4 text_color = hexToImVec4(userSettings.presentation.textColor);
                text_color.w *= presentation_fade_alpha;
                ImGui::PushStyleColor(ImGuiCol_Text, text_color);
                
                ImGui::TextWrapped("%s", current_displayed_verse.c_str());
                ImGui::Spacing();
                ImGui::Text("%s", current_displayed_reference.c_str());
                
                ImGui::PopStyleColor();
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "[Blank Screen]");
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
    
    // Control buttons
    if (ImGui::Button("Clear Display")) {
        clearDisplay();
    }
    ImGui::SameLine();
    
    if (ImGui::Button(presentation_blank_screen ? "Unblank" : "Blank Screen")) {
        toggleBlankScreen();
    }
}

void PresentationWindow::setupPresentationStyle() {
    // Setup ImGui style for presentation
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowBorderSize = 0.0f;
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.WindowPadding = ImVec2(0, 0);
    style.FramePadding = ImVec2(0, 0);
    style.ItemSpacing = ImVec2(0, 0);
    
    // Set font scale for presentation
    ImGui::GetIO().FontGlobalScale = userSettings.presentation.fontSize / 16.0f;
}

void PresentationWindow::calculateTextLayout(const std::string& text, float& wrap_width) {
    // This is a helper function to calculate optimal text layout
    // Currently just uses the provided wrap_width
    // Could be enhanced with more sophisticated layout calculations
}

void PresentationWindow::togglePresentationMode() {
    if (presentation_mode_active) {
        destroyPresentationWindow();
    } else {
        // Note: This needs the main window context to be passed
        // For now, just set the flag
        presentation_mode_active = true;
    }
}

void PresentationWindow::displayVerse(const std::string& verse_text, const std::string& reference) {
    current_displayed_verse = verse_text;
    current_displayed_reference = reference;
    presentation_blank_screen = false;
    presentation_fade_alpha = 1.0f;
    
    // Start text animation if enabled
    if (animation_system.isTextAnimationActive()) {
        animation_system.stopTextAnimation();
    }
    
    // Start default text animation
    animation_system.startTextAnimation(verse_text, TextAnimationType::FADE_IN, 1500.0f);
}

void PresentationWindow::clearDisplay() {
    current_displayed_verse.clear();
    current_displayed_reference.clear();
    presentation_blank_screen = false;
}

void PresentationWindow::toggleBlankScreen() {
    presentation_blank_screen = !presentation_blank_screen;
}

void PresentationWindow::updateMonitorPosition() {
    if (!presentation_window) return;
    
    int monitor_count;
    GLFWmonitor** monitors = glfwGetMonitors(&monitor_count);
    
    if (monitor_count >= 2) {
        GLFWmonitor* presentation_monitor = monitors[std::min(1, monitor_count - 1)];
        int monitor_x, monitor_y;
        glfwGetMonitorPos(presentation_monitor, &monitor_x, &monitor_y);
        glfwSetWindowPos(presentation_window, monitor_x, monitor_y);
    }
}

ImVec4 PresentationWindow::hexToImVec4(const std::string& hex_color) {
    // Convert hex color string (like "#FFFFFF" or "FFFFFF") to ImVec4
    std::string hex = hex_color;
    if (hex.empty()) {
        return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default to white
    }
    
    // Remove '#' if present
    if (hex[0] == '#') {
        hex = hex.substr(1);
    }
    
    // Ensure we have at least 6 characters (RGB)
    if (hex.length() < 6) {
        return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default to white
    }
    
    try {
        unsigned int color = std::stoul(hex, nullptr, 16);
        float r = ((color >> 16) & 0xFF) / 255.0f;
        float g = ((color >> 8) & 0xFF) / 255.0f;
        float b = (color & 0xFF) / 255.0f;
        float a = 1.0f;
        
        // If we have alpha channel (8 characters), parse it
        if (hex.length() >= 8) {
            a = ((std::stoul(hex.substr(6, 2), nullptr, 16)) & 0xFF) / 255.0f;
        }
        
        return ImVec4(r, g, b, a);
    } catch (const std::exception&) {
        return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default to white on error
    }
}

// Enhanced presentation methods
void PresentationWindow::startTransition(TransitionType type, float duration) {
    animation_system.startTransition(type, duration);
}

void PresentationWindow::startTextAnimation(TextAnimationType type, float duration) {
    if (!current_displayed_verse.empty()) {
        animation_system.startTextAnimation(current_displayed_verse, type, duration);
    }
}

void PresentationWindow::applyTextEffects(const std::string& preset) {
    presentation_effects.loadPreset(preset);
}

void PresentationWindow::setBackground(const BackgroundConfig& config) {
    media_manager.setBackground(config);
}

void PresentationWindow::startKenBurnsEffect(float duration) {
    animation_system.startKenBurnsEffect(1.0f, 1.1f, 0.0f, 0.0f, duration);
}

bool PresentationWindow::isAnimationActive() const {
    return animation_system.isTransitionActive() || 
           animation_system.isTextAnimationActive() || 
           animation_system.isKenBurnsActive();
}

void PresentationWindow::renderBackground() {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 display_size = io.DisplaySize;
    
    // Render background using MediaManager
    media_manager.renderCurrentBackground(ImVec2(0, 0), display_size);
}

void PresentationWindow::renderEnhancedPresentationContent() {
    // Create fullscreen window for content
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | 
                                   ImGuiWindowFlags_NoMove | 
                                   ImGuiWindowFlags_NoResize | 
                                   ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoBackground;
    
    if (ImGui::Begin("Enhanced Presentation Content", nullptr, window_flags)) {
        // Calculate text positioning
        float window_width = io.DisplaySize.x;
        float window_height = io.DisplaySize.y;
        
        // Get animated text if animation is active
        std::string display_verse = animation_system.isTextAnimationActive() ? 
                                   animation_system.getAnimatedText() : current_displayed_verse;
        
        // Calculate text size with wrapping
        float wrap_width = window_width * 0.9f; // 90% of window width
        calculateTextLayout(display_verse, wrap_width);
        
        ImVec2 text_size = ImGui::CalcTextSize(display_verse.c_str(), nullptr, false, wrap_width);
        ImVec2 ref_size = ImGui::CalcTextSize(current_displayed_reference.c_str());
        
        // Total content height
        float total_height = text_size.y + ref_size.y + 40; // 40px spacing
        
        // Center vertically with transition offset
        float start_y = (window_height - total_height) * 0.5f;
        
        // Apply transition effects
        if (animation_system.isTransitionActive()) {
            float progress = animation_system.getTransitionProgress();
            TransitionType type = animation_system.getCurrentTransitionType();
            
            switch (type) {
                case TransitionType::SLIDE_UP:
                    start_y += (1.0f - progress) * window_height;
                    break;
                case TransitionType::SLIDE_DOWN:
                    start_y -= (1.0f - progress) * window_height;
                    break;
                case TransitionType::SLIDE_LEFT:
                    // Implement horizontal sliding
                    break;
                case TransitionType::SLIDE_RIGHT:
                    // Implement horizontal sliding
                    break;
                case TransitionType::FADE:
                    presentation_fade_alpha = progress;
                    break;
                default:
                    break;
            }
        }
        
        // Render verse with effects
        ImVec2 verse_pos = ImVec2((window_width - wrap_width) * 0.5f, start_y);
        renderVerseWithEffects(verse_pos, ImVec2(wrap_width, text_size.y));
        
        // Render reference with effects
        ImVec2 ref_pos = ImVec2((window_width - ref_size.x) * 0.5f, start_y + text_size.y + 20);
        renderReferenceWithEffects(ref_pos, ref_size);
    }
    ImGui::End();
}

void PresentationWindow::renderVerseWithEffects(const ImVec2& position, const ImVec2& size) {
    std::string display_verse = animation_system.isTextAnimationActive() ? 
                               animation_system.getAnimatedText() : current_displayed_verse;
    
    // Apply text effects
    presentation_effects.beginTextEffects(position, size, display_verse, nullptr, 0.0f);
    presentation_effects.endTextEffects();
}

void PresentationWindow::renderReferenceWithEffects(const ImVec2& position, const ImVec2& size) {
    // Apply lighter effects to reference text
    presentation_effects.beginTextEffects(position, size, current_displayed_reference, nullptr, 0.0f);
    presentation_effects.endTextEffects();
}