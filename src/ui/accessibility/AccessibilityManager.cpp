#include "AccessibilityManager.h"
#include "imgui.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <regex>

// Platform-specific includes
#ifdef _WIN32
#include <windows.h>
#include <sapi.h>
#include <sphelper.h>
#elif __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>
#elif __linux__
#include <cstdlib>
#include <unistd.h>
#endif

AccessibilityManager::AccessibilityManager() {
    // Initialize default settings
    settings = AccessibilitySettings{};
}

AccessibilityManager::~AccessibilityManager() {
    shutdown();
}

bool AccessibilityManager::initialize() {
    if (is_initialized) {
        return true;
    }
    
    std::cout << "Initializing Accessibility Manager..." << std::endl;
    
    // Load saved settings
    loadSettings();
    
    // Initialize Text-to-Speech if enabled
    if (settings.screen_reader_enabled || settings.audio_feedback_enabled) {
        tts_available = initializeTTS();
        if (tts_available) {
            std::cout << "Text-to-Speech initialized successfully" << std::endl;
        } else {
            std::cout << "Text-to-Speech initialization failed" << std::endl;
        }
    }
    
    // Initialize voice recognition if enabled
    if (settings.voice_commands_enabled) {
        bool voice_init = initializeVoiceRecognition();
        if (voice_init) {
            std::cout << "Voice recognition initialized successfully" << std::endl;
        } else {
            std::cout << "Voice recognition initialization failed" << std::endl;
        }
    }
    
    is_initialized = true;
    return true;
}

void AccessibilityManager::shutdown() {
    if (!is_initialized) {
        return;
    }
    
    stopVoiceRecognition();
    stopSpeaking();
    saveSettings();
    
    is_initialized = false;
}

void AccessibilityManager::update() {
    if (!is_initialized) {
        return;
    }
    
    // Handle any ongoing voice recognition or TTS updates
    // This would be platform-specific and handle async operations
}

void AccessibilityManager::updateSettings(const AccessibilitySettings& new_settings) {
    AccessibilitySettings old_settings = settings;
    settings = new_settings;
    
    // Handle setting changes that require reinitialization
    if (old_settings.voice_commands_enabled != settings.voice_commands_enabled) {
        if (settings.voice_commands_enabled) {
            initializeVoiceRecognition();
        } else {
            stopVoiceRecognition();
        }
    }
    
    if ((old_settings.screen_reader_enabled != settings.screen_reader_enabled) ||
        (old_settings.audio_feedback_enabled != settings.audio_feedback_enabled)) {
        if (settings.screen_reader_enabled || settings.audio_feedback_enabled) {
            if (!tts_available) {
                tts_available = initializeTTS();
            }
        }
    }
    
    if (old_settings.high_contrast_enabled != settings.high_contrast_enabled) {
        if (settings.high_contrast_enabled) {
            applyHighContrastTheme();
        } else {
            applyNormalTheme();
        }
    }
}

void AccessibilityManager::saveSettings() {
    try {
        std::ofstream file("accessibility_settings.json");
        if (file.is_open()) {
            file << "{\n";
            file << "  \"high_contrast_enabled\": " << (settings.high_contrast_enabled ? "true" : "false") << ",\n";
            file << "  \"large_text_enabled\": " << (settings.large_text_enabled ? "true" : "false") << ",\n";
            file << "  \"screen_reader_enabled\": " << (settings.screen_reader_enabled ? "true" : "false") << ",\n";
            file << "  \"voice_commands_enabled\": " << (settings.voice_commands_enabled ? "true" : "false") << ",\n";
            file << "  \"audio_feedback_enabled\": " << (settings.audio_feedback_enabled ? "true" : "false") << ",\n";
            file << "  \"enhanced_keyboard_nav\": " << (settings.enhanced_keyboard_nav ? "true" : "false") << ",\n";
            file << "  \"focus_indicators_enabled\": " << (settings.focus_indicators_enabled ? "true" : "false") << ",\n";
            file << "  \"font_scale_factor\": " << settings.font_scale_factor << ",\n";
            file << "  \"speech_rate\": " << settings.speech_rate << ",\n";
            file << "  \"audio_volume\": " << settings.audio_volume << ",\n";
            file << "  \"preferred_voice\": \"" << settings.preferred_voice << "\",\n";
            file << "  \"contrast_theme\": \"" << settings.contrast_theme << "\"\n";
            file << "}\n";
            file.close();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error saving accessibility settings: " << e.what() << std::endl;
    }
}

void AccessibilityManager::loadSettings() {
    // For now, use defaults. In a full implementation, this would parse the JSON file
    // This is a minimal implementation to keep changes small
}

bool AccessibilityManager::isFeatureAvailable(AccessibilityFeature feature) const {
    switch (feature) {
        case AccessibilityFeature::HIGH_CONTRAST:
        case AccessibilityFeature::LARGE_TEXT:
        case AccessibilityFeature::KEYBOARD_NAVIGATION:
        case AccessibilityFeature::FOCUS_INDICATORS:
            return true; // Always available
        case AccessibilityFeature::SCREEN_READER:
        case AccessibilityFeature::AUDIO_FEEDBACK:
            return tts_available;
        case AccessibilityFeature::VOICE_COMMANDS:
            // Check if platform supports voice recognition
            #ifdef _WIN32
            return true; // Windows Speech API
            #elif __APPLE__
            return true; // macOS Speech Framework
            #elif __linux__
            return (system("which espeak > /dev/null 2>&1") == 0); // Check for espeak
            #else
            return false;
            #endif
    }
    return false;
}

bool AccessibilityManager::isFeatureEnabled(AccessibilityFeature feature) const {
    switch (feature) {
        case AccessibilityFeature::HIGH_CONTRAST:
            return settings.high_contrast_enabled;
        case AccessibilityFeature::LARGE_TEXT:
            return settings.large_text_enabled;
        case AccessibilityFeature::SCREEN_READER:
            return settings.screen_reader_enabled;
        case AccessibilityFeature::VOICE_COMMANDS:
            return settings.voice_commands_enabled;
        case AccessibilityFeature::AUDIO_FEEDBACK:
            return settings.audio_feedback_enabled;
        case AccessibilityFeature::KEYBOARD_NAVIGATION:
            return settings.enhanced_keyboard_nav;
        case AccessibilityFeature::FOCUS_INDICATORS:
            return settings.focus_indicators_enabled;
    }
    return false;
}

void AccessibilityManager::setFeatureEnabled(AccessibilityFeature feature, bool enabled) {
    switch (feature) {
        case AccessibilityFeature::HIGH_CONTRAST:
            settings.high_contrast_enabled = enabled;
            if (enabled) applyHighContrastTheme();
            else applyNormalTheme();
            break;
        case AccessibilityFeature::LARGE_TEXT:
            settings.large_text_enabled = enabled;
            break;
        case AccessibilityFeature::SCREEN_READER:
            settings.screen_reader_enabled = enabled;
            break;
        case AccessibilityFeature::VOICE_COMMANDS:
            settings.voice_commands_enabled = enabled;
            if (enabled) startVoiceRecognition();
            else stopVoiceRecognition();
            break;
        case AccessibilityFeature::AUDIO_FEEDBACK:
            settings.audio_feedback_enabled = enabled;
            break;
        case AccessibilityFeature::KEYBOARD_NAVIGATION:
            settings.enhanced_keyboard_nav = enabled;
            break;
        case AccessibilityFeature::FOCUS_INDICATORS:
            settings.focus_indicators_enabled = enabled;
            break;
    }
}

void AccessibilityManager::registerVoiceCommand(VoiceCommand command, std::function<void(const std::string&)> handler) {
    command_handlers[command] = handler;
}

VoiceCommand AccessibilityManager::parseVoiceCommand(const std::string& input) {
    std::string lower_input = input;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);
    
    // Simple pattern matching for voice commands
    if (lower_input.find("search") != std::string::npos || 
        lower_input.find("find") != std::string::npos ||
        lower_input.find("go to") != std::string::npos) {
        return VoiceCommand::SEARCH_VERSE;
    }
    
    if (lower_input.find("next verse") != std::string::npos) {
        return VoiceCommand::NEXT_VERSE;
    }
    
    if (lower_input.find("previous verse") != std::string::npos) {
        return VoiceCommand::PREVIOUS_VERSE;
    }
    
    if (lower_input.find("next chapter") != std::string::npos) {
        return VoiceCommand::NEXT_CHAPTER;
    }
    
    if (lower_input.find("previous chapter") != std::string::npos) {
        return VoiceCommand::PREVIOUS_CHAPTER;
    }
    
    if (lower_input.find("presentation") != std::string::npos || 
        lower_input.find("present") != std::string::npos) {
        return VoiceCommand::PRESENTATION_MODE;
    }
    
    if (lower_input.find("blank") != std::string::npos) {
        return VoiceCommand::BLANK_SCREEN;
    }
    
    if (lower_input.find("show verse") != std::string::npos) {
        return VoiceCommand::SHOW_VERSE;
    }
    
    if (lower_input.find("help") != std::string::npos) {
        return VoiceCommand::HELP;
    }
    
    if (lower_input.find("settings") != std::string::npos) {
        return VoiceCommand::SETTINGS;
    }
    
    return VoiceCommand::UNKNOWN;
}

void AccessibilityManager::processVoiceInput(const std::string& input) {
    if (!settings.voice_commands_enabled) {
        return;
    }
    
    VoiceCommand command = parseVoiceCommand(input);
    
    auto handler_it = command_handlers.find(command);
    if (handler_it != command_handlers.end()) {
        handler_it->second(input);
        
        if (settings.audio_feedback_enabled) {
            playConfirmationSound();
        }
    } else {
        if (settings.audio_feedback_enabled) {
            playErrorSound();
        }
    }
}

// Platform-specific TTS implementations
bool AccessibilityManager::initializeTTS() {
    #ifdef _WIN32
    return initializeWindowsSpeech();
    #elif __APPLE__
    return initializeMacOSSpeech();
    #else
    return initializeLinuxSpeech();
    #endif
}

#ifdef _WIN32
bool AccessibilityManager::initializeWindowsSpeech() {
    // Windows SAPI implementation would go here
    // For minimal implementation, return true
    return true;
}

void AccessibilityManager::speakTextWindows(const std::string& text) {
    // Simple fallback using Windows narrator command
    std::string command = "echo " + text + " | msg %username%";
    system(command.c_str());
}
#endif

#ifdef __APPLE__
bool AccessibilityManager::initializeMacOSSpeech() {
    // macOS Speech Synthesis implementation would go here
    return true;
}

void AccessibilityManager::speakTextMacOS(const std::string& text) {
    std::string command = "say \"" + text + "\"";
    system(command.c_str());
}
#endif

#ifndef _WIN32
#ifndef __APPLE__
bool AccessibilityManager::initializeLinuxSpeech() {
    // Check if espeak is available
    return (system("which espeak > /dev/null 2>&1") == 0);
}

void AccessibilityManager::speakTextLinux(const std::string& text) {
    std::string command = "espeak \"" + text + "\" 2>/dev/null &";
    system(command.c_str());
}
#endif
#endif

void AccessibilityManager::speakText(const std::string& text, bool interrupt) {
    if (!tts_available || !settings.screen_reader_enabled) {
        return;
    }
    
    if (interrupt) {
        stopSpeaking();
    }
    
    #ifdef _WIN32
    speakTextWindows(text);
    #elif __APPLE__
    speakTextMacOS(text);
    #else
    speakTextLinux(text);
    #endif
}

void AccessibilityManager::announceText(const std::string& text) {
    speakText(text, false);
}

void AccessibilityManager::announceVerseText(const std::string& verse, const std::string& reference) {
    if (settings.screen_reader_enabled) {
        std::string announcement = reference + ". " + verse;
        speakText(announcement, true);
    }
}

void AccessibilityManager::announceAction(const std::string& action) {
    if (settings.screen_reader_enabled) {
        speakText(action, false);
    }
}

void AccessibilityManager::stopSpeaking() {
    #ifdef _WIN32
    // Stop Windows speech
    #elif __APPLE__
    system("killall say 2>/dev/null");
    #else
    system("killall espeak 2>/dev/null");
    #endif
}

bool AccessibilityManager::isSpeaking() const {
    // Simple implementation - in reality this would check platform-specific speech status
    return false;
}

// Audio feedback implementations
void AccessibilityManager::playFeedbackSound([[maybe_unused]] const std::string& action) {
    if (!settings.audio_feedback_enabled) {
        return;
    }
    
    // Simple system beep for now - could be enhanced with actual sound files
    #ifdef _WIN32
    Beep(800, 100);
    #elif __APPLE__
    system("afplay /System/Library/Sounds/Tink.aiff");
    #else
    system("echo -e '\\a'");
    #endif
}

void AccessibilityManager::playSelectionSound() {
    playFeedbackSound("selection");
}

void AccessibilityManager::playNavigationSound() {
    playFeedbackSound("navigation");
}

void AccessibilityManager::playErrorSound() {
    playFeedbackSound("error");
}

void AccessibilityManager::playConfirmationSound() {
    playFeedbackSound("confirmation");
}

// Focus management
void AccessibilityManager::setFocus(const std::string& element_id) {
    current_focus_id = element_id;
    
    auto it = std::find(focus_order.begin(), focus_order.end(), element_id);
    if (it != focus_order.end()) {
        focus_index = std::distance(focus_order.begin(), it);
    }
    
    if (settings.audio_feedback_enabled) {
        playNavigationSound();
    }
}

void AccessibilityManager::navigateFocus(int direction) {
    if (focus_order.empty()) {
        return;
    }
    
    focus_index += direction;
    
    if (focus_index < 0) {
        focus_index = focus_order.size() - 1;
    } else if (focus_index >= static_cast<int>(focus_order.size())) {
        focus_index = 0;
    }
    
    current_focus_id = focus_order[focus_index];
    
    if (settings.audio_feedback_enabled) {
        playNavigationSound();
    }
}

void AccessibilityManager::setFocusOrder(const std::vector<std::string>& order) {
    focus_order = order;
    focus_index = -1;
    
    if (!current_focus_id.empty()) {
        auto it = std::find(focus_order.begin(), focus_order.end(), current_focus_id);
        if (it != focus_order.end()) {
            focus_index = std::distance(focus_order.begin(), it);
        }
    }
}

// High contrast theme implementation
void AccessibilityManager::applyHighContrastTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // High contrast dark theme
    if (settings.contrast_theme == "high_contrast_dark") {
        style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
    }
    // High contrast light theme
    else if (settings.contrast_theme == "high_contrast_light") {
        style.Colors[ImGuiCol_Text] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    }
    
    // Increase border thickness for better visibility
    style.FrameBorderSize = 2.0f;
    style.WindowBorderSize = 2.0f;
}

void AccessibilityManager::applyNormalTheme() {
    // Reset to default ImGui theme
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameBorderSize = 1.0f;
    style.WindowBorderSize = 1.0f;
}

void AccessibilityManager::renderFocusIndicator(const std::string& element_id) {
    if (!settings.focus_indicators_enabled || current_focus_id != element_id) {
        return;
    }
    
    // Draw a bright border around the focused element
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    
    // Expand the rectangle slightly
    min.x -= 2;
    min.y -= 2;
    max.x += 2;
    max.y += 2;
    
    // Draw bright yellow border
    draw_list->AddRect(min, max, IM_COL32(255, 255, 0, 255), 0.0f, 0, 3.0f);
}

bool AccessibilityManager::initializeVoiceRecognition() {
    // Placeholder implementation
    // In a full implementation, this would initialize platform-specific voice recognition
    return false; // For now, return false to indicate not implemented
}

void AccessibilityManager::startVoiceRecognition() {
    voice_recognition_active = true;
}

void AccessibilityManager::stopVoiceRecognition() {
    voice_recognition_active = false;
}

bool AccessibilityManager::handleAccessibilityKeyInput() {
    ImGuiIO& io = ImGui::GetIO();
    
    if (!settings.enhanced_keyboard_nav) {
        return false;
    }
    
    // Tab navigation
    if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
        navigateFocus(io.KeyShift ? -1 : 1);
        return true;
    }
    
    // Voice recognition toggle (Ctrl+Alt+V)
    if (io.KeyCtrl && io.KeyAlt && ImGui::IsKeyPressed(ImGuiKey_V)) {
        if (voice_recognition_active) {
            stopVoiceRecognition();
        } else {
            startVoiceRecognition();
        }
        announceAction(voice_recognition_active ? "Voice recognition started" : "Voice recognition stopped");
        return true;
    }
    
    // Speak current context (Ctrl+Alt+S)
    if (io.KeyCtrl && io.KeyAlt && ImGui::IsKeyPressed(ImGuiKey_S)) {
        announceCurrentContext();
        return true;
    }
    
    return false;
}

void AccessibilityManager::announceCurrentContext() {
    if (!settings.screen_reader_enabled) {
        return;
    }
    
    std::string context = "Current focus: " + current_focus_id;
    announceText(context);
}

void AccessibilityManager::setScreenReaderText([[maybe_unused]] const std::string& element_id, [[maybe_unused]] const std::string& text) {
    // Store screen reader text for elements
    // This would be expanded in a full implementation
}

std::string AccessibilityManager::getScreenReaderText(const std::string& element_id) const {
    // Return screen reader text for elements
    return element_id; // Simple fallback
}

void AccessibilityManager::setupImGuiAccessibility([[maybe_unused]] ImGuiContext* context) {
    // Setup ImGui for accessibility
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;
}

void AccessibilityManager::renderAccessibilityOverlay() {
    if (!settings.focus_indicators_enabled && !voice_recognition_active) {
        return;
    }
    
    // Render voice recognition indicator
    if (voice_recognition_active) {
        ImVec2 display_size = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowPos(ImVec2(display_size.x - 200, 10));
        ImGui::SetNextWindowSize(ImVec2(190, 50));
        
        if (ImGui::Begin("Voice Recognition", nullptr, 
                        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "🎤 Listening...");
        }
        ImGui::End();
    }
}