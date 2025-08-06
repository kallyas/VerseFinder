#ifndef ANIMATION_SYSTEM_H
#define ANIMATION_SYSTEM_H

#include <chrono>
#include <functional>
#include <vector>
#include <memory>

enum class TransitionType {
    FADE,
    SLIDE_LEFT,
    SLIDE_RIGHT,
    SLIDE_UP,
    SLIDE_DOWN,
    ZOOM_IN,
    ZOOM_OUT,
    CROSS_FADE
};

enum class EasingType {
    LINEAR,
    EASE_IN,
    EASE_OUT,
    EASE_IN_OUT,
    BOUNCE,
    ELASTIC
};

enum class TextAnimationType {
    FADE_IN,
    TYPE_ON,
    WORD_BY_WORD,
    LINE_BY_LINE,
    SLIDE_IN_LEFT,
    SLIDE_IN_RIGHT
};

struct Animation {
    std::chrono::steady_clock::time_point start_time;
    float duration;
    float current_value;
    float start_value;
    float end_value;
    EasingType easing;
    std::function<void(float)> update_callback;
    std::function<void()> complete_callback;
    bool active;
    bool completed;

    Animation(float start, float end, float duration_ms, EasingType easing_type)
        : duration(duration_ms), current_value(start), start_value(start), 
          end_value(end), easing(easing_type), active(false), completed(false) {}
};

struct TransitionAnimation {
    TransitionType type;
    float duration;
    EasingType easing;
    bool active;
    std::chrono::steady_clock::time_point start_time;
    float progress;

    TransitionAnimation(TransitionType t, float d, EasingType e) 
        : type(t), duration(d), easing(e), active(false), progress(0.0f) {}
};

struct TextAnimation {
    TextAnimationType type;
    float duration;
    float char_delay;
    float word_delay;
    float line_delay;
    bool active;
    std::chrono::steady_clock::time_point start_time;
    int current_char;
    int current_word;
    int current_line;
    std::string text;

    TextAnimation(TextAnimationType t, float d) 
        : type(t), duration(d), char_delay(0.05f), word_delay(0.1f), 
          line_delay(0.2f), active(false), current_char(0), 
          current_word(0), current_line(0) {}
};

class AnimationSystem {
public:
    AnimationSystem();
    ~AnimationSystem();

    // Core animation management
    void update();
    void reset();
    
    // Transition animations
    void startTransition(TransitionType type, float duration = 1000.0f, EasingType easing = EasingType::EASE_IN_OUT);
    void stopTransition();
    bool isTransitionActive() const { return transition && transition->active; }
    float getTransitionProgress() const;
    TransitionType getCurrentTransitionType() const;
    
    // Text animations
    void startTextAnimation(const std::string& text, TextAnimationType type, float duration = 2000.0f);
    void stopTextAnimation();
    bool isTextAnimationActive() const { return text_animation && text_animation->active; }
    std::string getAnimatedText() const;
    float getTextAnimationProgress() const;
    
    // Value animations
    std::shared_ptr<Animation> animateValue(float start, float end, float duration, 
                                          EasingType easing = EasingType::EASE_IN_OUT);
    void stopAnimation(std::shared_ptr<Animation> animation);
    
    // Ken Burns effect (slow zoom/pan for images)
    void startKenBurnsEffect(float zoom_start = 1.0f, float zoom_end = 1.1f, 
                           float pan_x = 0.0f, float pan_y = 0.0f, float duration = 10000.0f);
    void stopKenBurnsEffect();
    bool isKenBurnsActive() const { return ken_burns_active; }
    float getKenBurnsZoom() const { return ken_burns_zoom; }
    float getKenBurnsPanX() const { return ken_burns_pan_x; }
    float getKenBurnsPanY() const { return ken_burns_pan_y; }
    
    // Particle effects
    void startParticleEffect(const std::string& effect_type);
    void stopParticleEffect();
    bool isParticleEffectActive() const { return particle_effect_active; }
    
    // Easing functions
    static float easeLinear(float t);
    static float easeInQuad(float t);
    static float easeOutQuad(float t);
    static float easeInOutQuad(float t);
    static float easeBounce(float t);
    static float easeElastic(float t);
    
private:
    std::vector<std::shared_ptr<Animation>> animations;
    std::unique_ptr<TransitionAnimation> transition;
    std::unique_ptr<TextAnimation> text_animation;
    
    // Ken Burns effect state
    bool ken_burns_active;
    float ken_burns_zoom;
    float ken_burns_pan_x;
    float ken_burns_pan_y;
    std::shared_ptr<Animation> ken_burns_zoom_anim;
    std::shared_ptr<Animation> ken_burns_pan_x_anim;
    std::shared_ptr<Animation> ken_burns_pan_y_anim;
    
    // Particle effects
    bool particle_effect_active;
    std::string current_particle_effect;
    std::chrono::steady_clock::time_point particle_start_time;
    
    // Helper methods
    float applyEasing(float t, EasingType easing);
    void updateTransition();
    void updateTextAnimation();
    void updateValueAnimations();
    void updateKenBurnsEffect();
    void updateParticleEffects();
    
    // Text animation helpers
    int calculateVisibleChars() const;
    int calculateVisibleWords() const;
    int calculateVisibleLines() const;
};

#endif // ANIMATION_SYSTEM_H