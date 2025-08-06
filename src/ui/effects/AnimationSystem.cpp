#include "AnimationSystem.h"
#include <algorithm>
#include <cmath>
#include <sstream>

AnimationSystem::AnimationSystem()
    : ken_burns_active(false), ken_burns_zoom(1.0f), ken_burns_pan_x(0.0f), ken_burns_pan_y(0.0f),
      particle_effect_active(false) {}

AnimationSystem::~AnimationSystem() {
    reset();
}

void AnimationSystem::update() {
    updateTransition();
    updateTextAnimation();
    updateValueAnimations();
    updateKenBurnsEffect();
    updateParticleEffects();
}

void AnimationSystem::reset() {
    stopTransition();
    stopTextAnimation();
    stopKenBurnsEffect();
    stopParticleEffect();
    animations.clear();
}

// Transition animations
void AnimationSystem::startTransition(TransitionType type, float duration, EasingType easing) {
    stopTransition();
    transition = std::make_unique<TransitionAnimation>(type, duration, easing);
    transition->active = true;
    transition->start_time = std::chrono::steady_clock::now();
    transition->progress = 0.0f;
}

void AnimationSystem::stopTransition() {
    if (transition) {
        transition->active = false;
        transition.reset();
    }
}

float AnimationSystem::getTransitionProgress() const {
    if (!transition || !transition->active) return 1.0f;
    return transition->progress;
}

TransitionType AnimationSystem::getCurrentTransitionType() const {
    if (!transition || !transition->active) return TransitionType::FADE;
    return transition->type;
}

// Text animations
void AnimationSystem::startTextAnimation(const std::string& text, TextAnimationType type, float duration) {
    stopTextAnimation();
    text_animation = std::make_unique<TextAnimation>(type, duration);
    text_animation->text = text;
    text_animation->active = true;
    text_animation->start_time = std::chrono::steady_clock::now();
    text_animation->current_char = 0;
    text_animation->current_word = 0;
    text_animation->current_line = 0;
}

void AnimationSystem::stopTextAnimation() {
    if (text_animation) {
        text_animation->active = false;
        text_animation.reset();
    }
}

std::string AnimationSystem::getAnimatedText() const {
    if (!text_animation || !text_animation->active) return text_animation ? text_animation->text : "";
    
    switch (text_animation->type) {
        case TextAnimationType::TYPE_ON:
        case TextAnimationType::FADE_IN: {
            int visible_chars = calculateVisibleChars();
            return text_animation->text.substr(0, visible_chars);
        }
        case TextAnimationType::WORD_BY_WORD: {
            int visible_words = calculateVisibleWords();
            std::istringstream iss(text_animation->text);
            std::string word;
            std::string result;
            int word_count = 0;
            
            while (iss >> word && word_count < visible_words) {
                if (!result.empty()) result += " ";
                result += word;
                word_count++;
            }
            return result;
        }
        case TextAnimationType::LINE_BY_LINE: {
            int visible_lines = calculateVisibleLines();
            std::istringstream iss(text_animation->text);
            std::string line;
            std::string result;
            int line_count = 0;
            
            while (std::getline(iss, line) && line_count < visible_lines) {
                if (!result.empty()) result += "\n";
                result += line;
                line_count++;
            }
            return result;
        }
        default:
            return text_animation->text;
    }
}

float AnimationSystem::getTextAnimationProgress() const {
    if (!text_animation || !text_animation->active) return 1.0f;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - text_animation->start_time).count();
    return std::min(1.0f, static_cast<float>(elapsed) / text_animation->duration);
}

// Value animations
std::shared_ptr<Animation> AnimationSystem::animateValue(float start, float end, float duration, EasingType easing) {
    auto animation = std::make_shared<Animation>(start, end, duration, easing);
    animation->start_time = std::chrono::steady_clock::now();
    animation->active = true;
    animations.push_back(animation);
    return animation;
}

void AnimationSystem::stopAnimation(std::shared_ptr<Animation> animation) {
    if (animation) {
        animation->active = false;
        animation->completed = true;
    }
}

// Ken Burns effect
void AnimationSystem::startKenBurnsEffect(float zoom_start, float zoom_end, float pan_x, float pan_y, float duration) {
    stopKenBurnsEffect();
    ken_burns_active = true;
    
    ken_burns_zoom_anim = animateValue(zoom_start, zoom_end, duration, EasingType::LINEAR);
    ken_burns_pan_x_anim = animateValue(0.0f, pan_x, duration, EasingType::LINEAR);
    ken_burns_pan_y_anim = animateValue(0.0f, pan_y, duration, EasingType::LINEAR);
    
    ken_burns_zoom_anim->update_callback = [this](float value) { ken_burns_zoom = value; };
    ken_burns_pan_x_anim->update_callback = [this](float value) { ken_burns_pan_x = value; };
    ken_burns_pan_y_anim->update_callback = [this](float value) { ken_burns_pan_y = value; };
    
    ken_burns_zoom_anim->complete_callback = [this]() { ken_burns_active = false; };
}

void AnimationSystem::stopKenBurnsEffect() {
    ken_burns_active = false;
    if (ken_burns_zoom_anim) stopAnimation(ken_burns_zoom_anim);
    if (ken_burns_pan_x_anim) stopAnimation(ken_burns_pan_x_anim);
    if (ken_burns_pan_y_anim) stopAnimation(ken_burns_pan_y_anim);
    ken_burns_zoom = 1.0f;
    ken_burns_pan_x = 0.0f;
    ken_burns_pan_y = 0.0f;
}

// Particle effects
void AnimationSystem::startParticleEffect(const std::string& effect_type) {
    particle_effect_active = true;
    current_particle_effect = effect_type;
    particle_start_time = std::chrono::steady_clock::now();
}

void AnimationSystem::stopParticleEffect() {
    particle_effect_active = false;
    current_particle_effect.clear();
}

// Easing functions
float AnimationSystem::easeLinear(float t) {
    return t;
}

float AnimationSystem::easeInQuad(float t) {
    return t * t;
}

float AnimationSystem::easeOutQuad(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t);
}

float AnimationSystem::easeInOutQuad(float t) {
    return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

float AnimationSystem::easeBounce(float t) {
    const float n1 = 7.5625f;
    const float d1 = 2.75f;
    
    if (t < 1.0f / d1) {
        return n1 * t * t;
    } else if (t < 2.0f / d1) {
        t -= 1.5f / d1;
        return n1 * t * t + 0.75f;
    } else if (t < 2.5f / d1) {
        t -= 2.25f / d1;
        return n1 * t * t + 0.9375f;
    } else {
        t -= 2.625f / d1;
        return n1 * t * t + 0.984375f;
    }
}

float AnimationSystem::easeElastic(float t) {
    const float c4 = (2.0f * M_PI) / 3.0f;
    
    return t == 0.0f ? 0.0f : t == 1.0f ? 1.0f : 
           -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * c4);
}

// Private helper methods
float AnimationSystem::applyEasing(float t, EasingType easing) {
    switch (easing) {
        case EasingType::LINEAR: return easeLinear(t);
        case EasingType::EASE_IN: return easeInQuad(t);
        case EasingType::EASE_OUT: return easeOutQuad(t);
        case EasingType::EASE_IN_OUT: return easeInOutQuad(t);
        case EasingType::BOUNCE: return easeBounce(t);
        case EasingType::ELASTIC: return easeElastic(t);
        default: return easeLinear(t);
    }
}

void AnimationSystem::updateTransition() {
    if (!transition || !transition->active) return;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - transition->start_time).count();
    float t = static_cast<float>(elapsed) / transition->duration;
    
    if (t >= 1.0f) {
        transition->progress = 1.0f;
        transition->active = false;
    } else {
        transition->progress = applyEasing(t, transition->easing);
    }
}

void AnimationSystem::updateTextAnimation() {
    if (!text_animation || !text_animation->active) return;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - text_animation->start_time).count();
    
    if (elapsed >= text_animation->duration) {
        text_animation->active = false;
    }
}

void AnimationSystem::updateValueAnimations() {
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = animations.begin(); it != animations.end();) {
        auto& animation = *it;
        
        if (!animation->active || animation->completed) {
            it = animations.erase(it);
            continue;
        }
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - animation->start_time).count();
        float t = static_cast<float>(elapsed) / animation->duration;
        
        if (t >= 1.0f) {
            animation->current_value = animation->end_value;
            animation->active = false;
            animation->completed = true;
            
            if (animation->update_callback) {
                animation->update_callback(animation->current_value);
            }
            if (animation->complete_callback) {
                animation->complete_callback();
            }
        } else {
            float eased_t = applyEasing(t, animation->easing);
            animation->current_value = animation->start_value + 
                                     (animation->end_value - animation->start_value) * eased_t;
            
            if (animation->update_callback) {
                animation->update_callback(animation->current_value);
            }
        }
        
        ++it;
    }
}

void AnimationSystem::updateKenBurnsEffect() {
    // Ken Burns effect is handled by the individual value animations
    // This method is kept for future enhancement
}

void AnimationSystem::updateParticleEffects() {
    if (!particle_effect_active) return;
    
    // Basic particle effect timing - can be enhanced with actual particle rendering
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - particle_start_time).count();
    
    // Stop particle effects after 5 seconds by default
    if (elapsed > 5000) {
        stopParticleEffect();
    }
}

// Text animation helper methods
int AnimationSystem::calculateVisibleChars() const {
    if (!text_animation) return 0;
    
    float progress = getTextAnimationProgress();
    return static_cast<int>(progress * text_animation->text.length());
}

int AnimationSystem::calculateVisibleWords() const {
    if (!text_animation) return 0;
    
    std::istringstream iss(text_animation->text);
    std::string word;
    int total_words = 0;
    while (iss >> word) total_words++;
    
    float progress = getTextAnimationProgress();
    return static_cast<int>(progress * total_words);
}

int AnimationSystem::calculateVisibleLines() const {
    if (!text_animation) return 0;
    
    int total_lines = std::count(text_animation->text.begin(), text_animation->text.end(), '\n') + 1;
    float progress = getTextAnimationProgress();
    return static_cast<int>(progress * total_lines);
}