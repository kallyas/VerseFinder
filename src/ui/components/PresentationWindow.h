#ifndef PRESENTATION_WINDOW_H
#define PRESENTATION_WINDOW_H

#include <GLFW/glfw3.h>
#include <string>
#include <imgui.h>
#include "../../core/UserSettings.h"

class PresentationWindow {
public:
    PresentationWindow(UserSettings& settings);
    ~PresentationWindow();

    // Window management
    bool initPresentationWindow(GLFWwindow* main_window);
    void destroyPresentationWindow();
    bool isPresentationWindowActive() const { return presentation_window != nullptr; }
    
    // Rendering
    void renderPresentationWindow();
    void renderPresentationPreview();
    
    // Display control
    void togglePresentationMode();
    void displayVerse(const std::string& verse_text, const std::string& reference);
    void clearDisplay();
    void toggleBlankScreen();
    
    // Settings
    void updateMonitorPosition();
    
    // State queries
    bool isPresentationModeActive() const { return presentation_mode_active; }
    bool isBlankScreenActive() const { return presentation_blank_screen; }
    float getFadeAlpha() const { return presentation_fade_alpha; }
    
    // Current content
    const std::string& getCurrentDisplayedVerse() const { return current_displayed_verse; }
    const std::string& getCurrentDisplayedReference() const { return current_displayed_reference; }

private:
    UserSettings& userSettings;
    GLFWwindow* presentation_window;
    
    // Presentation state
    bool presentation_mode_active;
    std::string current_displayed_verse;
    std::string current_displayed_reference;
    float presentation_fade_alpha;
    bool presentation_blank_screen;
    
    // Helper methods
    void setupPresentationStyle();
    void renderPresentationContent();
    void calculateTextLayout(const std::string& text, float& wrap_width);
    ImVec4 hexToImVec4(const std::string& hex_color);
};

#endif // PRESENTATION_WINDOW_H