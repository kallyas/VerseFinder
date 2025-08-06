#ifndef PRESENTATION_WINDOW_H
#define PRESENTATION_WINDOW_H

#include <GLFW/glfw3.h>
#include <string>
#include <imgui.h>
#include "../../core/UserSettings.h"
#include "../effects/AnimationSystem.h"
#include "../effects/PresentationEffects.h"
#include "../effects/MediaManager.h"

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
    
    // Animation and effects
    void startTransition(TransitionType type, float duration = 1000.0f);
    void startTextAnimation(TextAnimationType type, float duration = 2000.0f);
    void applyTextEffects(const std::string& preset = "default");
    void setBackground(const BackgroundConfig& config);
    void startKenBurnsEffect(float duration = 10000.0f);
    
    // Settings
    void updateMonitorPosition();
    
    // State queries
    bool isPresentationModeActive() const { return presentation_mode_active; }
    bool isBlankScreenActive() const { return presentation_blank_screen; }
    float getFadeAlpha() const { return presentation_fade_alpha; }
    bool isAnimationActive() const;
    
    // Current content
    const std::string& getCurrentDisplayedVerse() const { return current_displayed_verse; }
    const std::string& getCurrentDisplayedReference() const { return current_displayed_reference; }
    
    // Effects access for configuration
    AnimationSystem& getAnimationSystem() { return animation_system; }
    PresentationEffects& getPresentationEffects() { return presentation_effects; }
    MediaManager& getMediaManager() { return media_manager; }

private:
    UserSettings& userSettings;
    GLFWwindow* presentation_window;
    
    // Effects systems
    AnimationSystem animation_system;
    PresentationEffects presentation_effects;
    MediaManager media_manager;
    
    // Presentation state
    bool presentation_mode_active;
    std::string current_displayed_verse;
    std::string current_displayed_reference;
    float presentation_fade_alpha;
    bool presentation_blank_screen;
    
    // Enhanced display state
    std::string animated_verse_text;
    std::string animated_reference_text;
    
    // Helper methods
    void setupPresentationStyle();
    void renderPresentationContent();
    void renderEnhancedPresentationContent();
    void calculateTextLayout(const std::string& text, float& wrap_width);
    ImVec4 hexToImVec4(const std::string& hex_color);
    
    // Background rendering
    void renderBackground();
    
    // Text rendering with effects
    void renderVerseWithEffects(const ImVec2& position, const ImVec2& size);
    void renderReferenceWithEffects(const ImVec2& position, const ImVec2& size);
};

#endif // PRESENTATION_WINDOW_H