#include <iostream>
#include <thread>
#include <chrono>
#include "src/ui/effects/AnimationSystem.h"

// Mock ImGui types for testing without GUI
struct MockImVec2 { float x, y; MockImVec2(float x, float y) : x(x), y(y) {} };
struct MockImVec4 { float x, y, z, w; MockImVec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {} };

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
    
    // Test different transition types
    std::cout << std::endl;
    std::cout << "Testing Different Transition Types..." << std::endl;
    std::vector<TransitionType> transitions = {
        TransitionType::FADE,
        TransitionType::SLIDE_LEFT,
        TransitionType::SLIDE_RIGHT,
        TransitionType::SLIDE_UP,
        TransitionType::SLIDE_DOWN,
        TransitionType::ZOOM_IN,
        TransitionType::ZOOM_OUT
    };
    
    std::vector<std::string> transitionNames = {
        "Fade", "Slide Left", "Slide Right", "Slide Up", "Slide Down", "Zoom In", "Zoom Out"
    };
    
    for (size_t i = 0; i < transitions.size(); i++) {
        std::cout << "  Testing " << transitionNames[i] << " transition..." << std::endl;
        animationSystem.startTransition(transitions[i], 1000.0f);
        
        for (int j = 0; j < 5; j++) {
            animationSystem.update();
            float progress = animationSystem.getTransitionProgress();
            std::cout << "    Progress: " << (progress * 100) << "%" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
    
    // Test different text animation types
    std::cout << std::endl;
    std::cout << "Testing Different Text Animation Types..." << std::endl;
    std::vector<TextAnimationType> textAnimations = {
        TextAnimationType::FADE_IN,
        TextAnimationType::TYPE_ON,
        TextAnimationType::WORD_BY_WORD,
        TextAnimationType::LINE_BY_LINE
    };
    
    std::vector<std::string> textAnimationNames = {
        "Fade In", "Type On", "Word by Word", "Line by Line"
    };
    
    std::string testVerse = "In the beginning was the Word, and the Word was with God, and the Word was God.";
    
    for (size_t i = 0; i < textAnimations.size(); i++) {
        std::cout << "  Testing " << textAnimationNames[i] << " text animation..." << std::endl;
        animationSystem.startTextAnimation(testVerse, textAnimations[i], 2000.0f);
        
        for (int j = 0; j < 10; j++) {
            animationSystem.update();
            std::string animated_text = animationSystem.getAnimatedText();
            float progress = animationSystem.getTextAnimationProgress();
            std::cout << "    (" << (progress * 100) << "%): \"" << animated_text << "\"" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
    
    std::cout << std::endl;
    std::cout << "=== Demo completed successfully! ===" << std::endl;
    std::cout << std::endl;
    std::cout << "Advanced Presentation Features Available:" << std::endl;
    std::cout << "✓ Smooth slide transitions (fade, slide, zoom)" << std::endl;
    std::cout << "✓ Text animation effects (fade in, type-on, word-by-word)" << std::endl;
    std::cout << "✓ Custom transition timing and easing" << std::endl;
    std::cout << "✓ Ken Burns effect for background images" << std::endl;
    std::cout << "✓ 6 different easing functions (linear, ease-in/out, bounce, elastic)" << std::endl;
    std::cout << "✓ Professional animation timing and smoothness" << std::endl;
    std::cout << "✓ Text effects and visual enhancements" << std::endl;
    std::cout << "✓ Background management capabilities" << std::endl;
    std::cout << "✓ Extensible architecture for future enhancements" << std::endl;
    
    return 0;
}