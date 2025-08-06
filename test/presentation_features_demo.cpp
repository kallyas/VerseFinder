#include <iostream>
#include <thread>
#include <chrono>
#include "src/ui/effects/AnimationSystem.h"
#include "src/ui/effects/PresentationEffects.h"
#include "src/ui/effects/MediaManager.h"

int main() {
    std::cout << "=== VerseFinder Advanced Presentation Features Demo ===" << std::endl;
    std::cout << std::endl;
    
    // Test Animation System
    std::cout << "Testing Animation System..." << std::endl;
    AnimationSystem animationSystem;
    
    // Test transitions
    std::cout << "  Starting fade transition..." << std::endl;
    animationSystem.startTransition(TransitionType::FADE, 2000.0f);
    
    // Simulate animation update loop
    for (int i = 0; i < 10; i++) {
        animationSystem.update();
        float progress = animationSystem.getTransitionProgress();
        std::cout << "    Progress: " << (progress * 100) << "%" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    // Test text animation
    std::cout << "  Starting text animation..." << std::endl;
    std::string verse = "For God so loved the world that he gave his one and only Son";
    animationSystem.startTextAnimation(verse, TextAnimationType::TYPE_ON, 3000.0f);
    
    for (int i = 0; i < 15; i++) {
        animationSystem.update();
        std::string animated_text = animationSystem.getAnimatedText();
        std::cout << "    Text: \"" << animated_text << "\"" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    // Test Ken Burns effect
    std::cout << "  Starting Ken Burns effect..." << std::endl;
    animationSystem.startKenBurnsEffect(1.0f, 1.2f, 10.0f, 5.0f, 5000.0f);
    
    for (int i = 0; i < 10; i++) {
        animationSystem.update();
        if (animationSystem.isKenBurnsActive()) {
            float zoom = animationSystem.getKenBurnsZoom();
            float pan_x = animationSystem.getKenBurnsPanX();
            float pan_y = animationSystem.getKenBurnsPanY();
            std::cout << "    Ken Burns - Zoom: " << zoom 
                     << ", Pan X: " << pan_x << ", Pan Y: " << pan_y << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    std::cout << std::endl;
    
    // Test Presentation Effects
    std::cout << "Testing Presentation Effects..." << std::endl;
    PresentationEffects effects;
    
    // Test different presets
    std::vector<std::string> presets = {"classic", "modern", "bold", "elegant"};
    for (const auto& preset : presets) {
        std::cout << "  Loading preset: " << preset << std::endl;
        effects.loadPreset(preset);
        
        // Display current effect settings
        auto& dropShadow = effects.getDropShadow();
        auto& outline = effects.getOutline();
        auto& glow = effects.getGlow();
        
        std::cout << "    Drop Shadow: " << (dropShadow.enabled ? "enabled" : "disabled") << std::endl;
        std::cout << "    Outline: " << (outline.enabled ? "enabled" : "disabled") << std::endl;
        std::cout << "    Glow: " << (glow.enabled ? "enabled" : "disabled") << std::endl;
    }
    
    std::cout << std::endl;
    
    // Test Media Manager
    std::cout << "Testing Media Manager..." << std::endl;
    MediaManager mediaManager;
    
    // Test background configuration
    std::cout << "  Setting up solid color background..." << std::endl;
    BackgroundConfig solidConfig;
    solidConfig.type = BackgroundType::SOLID_COLOR;
    solidConfig.colors.clear();
    solidConfig.colors.push_back(0xFF003366); // Dark blue
    mediaManager.setBackground(solidConfig);
    
    std::cout << "  Setting up gradient background..." << std::endl;
    BackgroundConfig gradientConfig;
    gradientConfig.type = BackgroundType::GRADIENT;
    gradientConfig.colors.clear();
    gradientConfig.colors.push_back(0xFF000000); // Black
    gradientConfig.colors.push_back(0xFF333333); // Dark gray
    gradientConfig.gradient_angle = 45.0f;
    mediaManager.setBackground(gradientConfig);
    
    // Test seasonal themes
    std::cout << "  Loading seasonal themes..." << std::endl;
    mediaManager.loadSeasonalThemes();
    auto activeThemes = mediaManager.getActiveSeasonalThemes();
    std::cout << "    Active seasonal themes: " << activeThemes.size() << std::endl;
    
    // Test supported formats
    std::cout << "  Supported image formats:" << std::endl;
    auto imageFormats = MediaManager::getSupportedImageFormats();
    for (const auto& format : imageFormats) {
        std::cout << "    " << format;
    }
    std::cout << std::endl;
    
    std::cout << "  Supported video formats:" << std::endl;
    auto videoFormats = MediaManager::getSupportedVideoFormats();
    for (const auto& format : videoFormats) {
        std::cout << "    " << format;
    }
    std::cout << std::endl;
    
    // Test easing functions
    std::cout << std::endl;
    std::cout << "Testing Easing Functions..." << std::endl;
    std::vector<std::pair<std::string, std::function<float(float)>>> easingFunctions = {
        {"Linear", AnimationSystem::easeLinear},
        {"Ease In", AnimationSystem::easeInQuad},
        {"Ease Out", AnimationSystem::easeOutQuad},
        {"Ease In-Out", AnimationSystem::easeInOutQuad},
        {"Bounce", AnimationSystem::easeBounce},
        {"Elastic", AnimationSystem::easeElastic}
    };
    
    for (const auto& [name, func] : easingFunctions) {
        std::cout << "  " << name << " easing: ";
        for (float t = 0.0f; t <= 1.0f; t += 0.25f) {
            std::cout << func(t) << " ";
        }
        std::cout << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "=== Demo completed successfully! ===" << std::endl;
    std::cout << std::endl;
    std::cout << "Advanced Presentation Features Available:" << std::endl;
    std::cout << "✓ Smooth slide transitions (fade, slide, zoom)" << std::endl;
    std::cout << "✓ Text animation effects (fade in, type-on, word-by-word)" << std::endl;
    std::cout << "✓ Custom transition timing and easing" << std::endl;
    std::cout << "✓ Ken Burns effect for background images" << std::endl;
    std::cout << "✓ Text effects (drop shadows, outlines, glow)" << std::endl;
    std::cout << "✓ Background management (solid, gradient, images)" << std::endl;
    std::cout << "✓ Seasonal theme support" << std::endl;
    std::cout << "✓ Media asset management" << std::endl;
    std::cout << "✓ Professional presentation presets" << std::endl;
    
    return 0;
}