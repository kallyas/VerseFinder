#ifndef ACCESSIBILITYMANAGER_H
#define ACCESSIBILITYMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include "../core/UserSettings.h"

// Forward declarations
struct ImGuiContext;

enum class AccessibilityFeature {
    HIGH_CONTRAST,
    LARGE_TEXT,
    SCREEN_READER,
    VOICE_COMMANDS,
    AUDIO_FEEDBACK,
    KEYBOARD_NAVIGATION,
    FOCUS_INDICATORS
};

enum class VoiceCommand {
    SEARCH_VERSE,
    NEXT_VERSE,
    PREVIOUS_VERSE,
    NEXT_CHAPTER,
    PREVIOUS_CHAPTER,
    PRESENTATION_MODE,
    BLANK_SCREEN,
    SHOW_VERSE,
    HELP,
    SETTINGS,
    UNKNOWN
};

class AccessibilityManager {
private:
    AccessibilitySettings settings;
    bool is_initialized = false;
    bool voice_recognition_active = false;
    bool tts_available = false;
    
    // Voice command callbacks
    std::map<VoiceCommand, std::function<void(const std::string&)>> command_handlers;
    
    // Focus management
    std::string current_focus_id;
    std::vector<std::string> focus_order;
    int focus_index = -1;
    
    // Audio feedback
    void playFeedbackSound(const std::string& action);
    
    // Voice recognition
    bool initializeVoiceRecognition();
    void processVoiceInput(const std::string& input);
    VoiceCommand parseVoiceCommand(const std::string& input);
    
    // Text-to-Speech
    bool initializeTTS();
    void speakText(const std::string& text, bool interrupt = false);
    
    // Platform-specific implementations
    #ifdef _WIN32
    bool initializeWindowsSpeech();
    void speakTextWindows(const std::string& text);
    #elif __APPLE__
    bool initializeMacOSSpeech();
    void speakTextMacOS(const std::string& text);
    #else
    bool initializeLinuxSpeech();
    void speakTextLinux(const std::string& text);
    #endif

public:
    AccessibilityManager();
    ~AccessibilityManager();
    
    // Core management
    bool initialize();
    void shutdown();
    void update(); // Called each frame to process voice input, etc.
    
    // Settings management
    const AccessibilitySettings& getSettings() const { return settings; }
    void updateSettings(const AccessibilitySettings& new_settings);
    void saveSettings();
    void loadSettings();
    
    // Feature availability
    bool isFeatureAvailable(AccessibilityFeature feature) const;
    bool isFeatureEnabled(AccessibilityFeature feature) const;
    void setFeatureEnabled(AccessibilityFeature feature, bool enabled);
    
    // Voice control
    void startVoiceRecognition();
    void stopVoiceRecognition();
    bool isVoiceRecognitionActive() const { return voice_recognition_active; }
    void registerVoiceCommand(VoiceCommand command, std::function<void(const std::string&)> handler);
    
    // Text-to-Speech
    void announceText(const std::string& text);
    void announceVerseText(const std::string& verse, const std::string& reference);
    void announceAction(const std::string& action);
    void stopSpeaking();
    bool isSpeaking() const;
    
    // Audio feedback
    void playSelectionSound();
    void playNavigationSound();
    void playErrorSound();
    void playConfirmationSound();
    
    // Focus management
    void setFocus(const std::string& element_id);
    std::string getCurrentFocus() const { return current_focus_id; }
    void navigateFocus(int direction); // -1 = previous, 1 = next
    void setFocusOrder(const std::vector<std::string>& order);
    
    // Keyboard navigation helpers
    bool handleAccessibilityKeyInput();
    bool isAccessibilityKeyCombo() const;
    
    // Screen reader support
    void announceCurrentContext();
    void setScreenReaderText(const std::string& element_id, const std::string& text);
    std::string getScreenReaderText(const std::string& element_id) const;
    
    // High contrast and theming support
    void applyHighContrastTheme();
    void applyNormalTheme();
    float getFontScaleFactor() const { return settings.font_scale_factor; }
    
    // ImGui integration
    void setupImGuiAccessibility(ImGuiContext* context);
    void renderAccessibilityOverlay();
    void renderFocusIndicator(const std::string& element_id);
};

#endif // ACCESSIBILITYMANAGER_H