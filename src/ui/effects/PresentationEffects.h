#ifndef PRESENTATION_EFFECTS_H
#define PRESENTATION_EFFECTS_H

#include <imgui.h>
#include <string>
#include <vector>
#include <memory>

struct TextEffect {
    bool enabled;
    ImVec4 color;
    float intensity;
    
    TextEffect() : enabled(false), color(1.0f, 1.0f, 1.0f, 1.0f), intensity(1.0f) {}
};

struct DropShadowEffect : public TextEffect {
    float offset_x;
    float offset_y;
    float blur_radius;
    
    DropShadowEffect() : offset_x(2.0f), offset_y(2.0f), blur_radius(4.0f) {}
};

struct OutlineEffect : public TextEffect {
    float thickness;
    
    OutlineEffect() : thickness(1.0f) {}
};

struct GlowEffect : public TextEffect {
    float radius;
    float strength;
    
    GlowEffect() : radius(10.0f), strength(0.5f) {}
};

struct GradientEffect : public TextEffect {
    ImVec4 start_color;
    ImVec4 end_color;
    float angle; // 0 = horizontal, 90 = vertical
    
    GradientEffect() : start_color(1.0f, 1.0f, 1.0f, 1.0f), 
                      end_color(0.8f, 0.8f, 0.8f, 1.0f), angle(90.0f) {}
};

struct StrokeEffect : public TextEffect {
    float width;
    ImVec4 stroke_color;
    
    StrokeEffect() : width(2.0f), stroke_color(0.0f, 0.0f, 0.0f, 1.0f) {}
};

struct BackgroundEffect {
    bool enabled;
    ImVec4 color;
    float opacity;
    float blur_radius;
    float padding_x;
    float padding_y;
    float corner_radius;
    
    BackgroundEffect() : enabled(false), color(0.0f, 0.0f, 0.0f, 0.7f), 
                        opacity(0.7f), blur_radius(0.0f), padding_x(20.0f), 
                        padding_y(10.0f), corner_radius(5.0f) {}
};

class PresentationEffects {
public:
    PresentationEffects();
    ~PresentationEffects();
    
    // Text effects
    void setDropShadow(bool enabled, float offset_x = 2.0f, float offset_y = 2.0f, 
                      float blur = 4.0f, ImVec4 color = ImVec4(0.0f, 0.0f, 0.0f, 0.8f));
    void setOutline(bool enabled, float thickness = 1.0f, 
                   ImVec4 color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    void setGlow(bool enabled, float radius = 10.0f, float strength = 0.5f, 
                ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 0.8f));
    void setGradient(bool enabled, ImVec4 start_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 
                    ImVec4 end_color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f), float angle = 90.0f);
    void setStroke(bool enabled, float width = 2.0f, 
                  ImVec4 stroke_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f), 
                  ImVec4 fill_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    
    // Background effects
    void setTextBackground(bool enabled, ImVec4 color = ImVec4(0.0f, 0.0f, 0.0f, 0.7f),
                          float padding_x = 20.0f, float padding_y = 10.0f, 
                          float corner_radius = 5.0f);
    
    // Rendering methods
    void beginTextEffects(const ImVec2& position, const ImVec2& size, const std::string& text, 
                         ImFont* font = nullptr, float font_size = 0.0f);
    void endTextEffects();
    
    void renderDropShadow(const ImVec2& position, const ImVec2& size, const std::string& text, 
                         ImFont* font, float font_size);
    void renderOutline(const ImVec2& position, const ImVec2& size, const std::string& text, 
                      ImFont* font, float font_size);
    void renderGlow(const ImVec2& position, const ImVec2& size, const std::string& text, 
                   ImFont* font, float font_size);
    void renderGradientText(const ImVec2& position, const ImVec2& size, const std::string& text, 
                           ImFont* font, float font_size);
    void renderStrokedText(const ImVec2& position, const ImVec2& size, const std::string& text, 
                          ImFont* font, float font_size);
    void renderTextBackground(const ImVec2& position, const ImVec2& size);
    
    // Configuration
    void resetEffects();
    void loadPreset(const std::string& preset_name);
    void savePreset(const std::string& preset_name);
    
    // Getters for configuration UI
    DropShadowEffect& getDropShadow() { return drop_shadow; }
    OutlineEffect& getOutline() { return outline; }
    GlowEffect& getGlow() { return glow; }
    GradientEffect& getGradient() { return gradient; }
    StrokeEffect& getStroke() { return stroke; }
    BackgroundEffect& getBackground() { return background; }
    
    // Utility methods
    static ImVec4 blendColors(ImVec4 color1, ImVec4 color2, float factor);
    static ImVec4 interpolateGradient(ImVec4 start, ImVec4 end, float position, float angle);
    static void drawBlurredRect(ImDrawList* draw_list, const ImVec2& min, const ImVec2& max, 
                               ImU32 color, float blur_radius, float corner_radius = 0.0f);
    
private:
    DropShadowEffect drop_shadow;
    OutlineEffect outline;
    GlowEffect glow;
    GradientEffect gradient;
    StrokeEffect stroke;
    BackgroundEffect background;
    
    // Rendering state
    ImDrawList* current_draw_list;
    ImVec2 current_position;
    ImVec2 current_size;
    std::string current_text;
    ImFont* current_font;
    float current_font_size;
    
    // Helper methods
    void renderTextWithColor(const ImVec2& position, const std::string& text, 
                           ImFont* font, float font_size, ImU32 color);
    void renderTextMultiple(const ImVec2& position, const std::string& text, 
                           ImFont* font, float font_size, ImU32 color, 
                           int offset_x, int offset_y, int samples = 8);
    ImU32 imVec4ToImU32(const ImVec4& color);
    float calculateFontSize(ImFont* font, float desired_size);
};

#endif // PRESENTATION_EFFECTS_H