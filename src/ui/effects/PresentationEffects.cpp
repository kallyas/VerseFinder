#include "PresentationEffects.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>

PresentationEffects::PresentationEffects() 
    : current_draw_list(nullptr), current_font(nullptr), current_font_size(0.0f) {}

PresentationEffects::~PresentationEffects() {}

// Text effects configuration
void PresentationEffects::setDropShadow(bool enabled, float offset_x, float offset_y, float blur, ImVec4 color) {
    drop_shadow.enabled = enabled;
    drop_shadow.offset_x = offset_x;
    drop_shadow.offset_y = offset_y;
    drop_shadow.blur_radius = blur;
    drop_shadow.color = color;
}

void PresentationEffects::setOutline(bool enabled, float thickness, ImVec4 color) {
    outline.enabled = enabled;
    outline.thickness = thickness;
    outline.color = color;
}

void PresentationEffects::setGlow(bool enabled, float radius, float strength, ImVec4 color) {
    glow.enabled = enabled;
    glow.radius = radius;
    glow.strength = strength;
    glow.color = color;
}

void PresentationEffects::setGradient(bool enabled, ImVec4 start_color, ImVec4 end_color, float angle) {
    gradient.enabled = enabled;
    gradient.start_color = start_color;
    gradient.end_color = end_color;
    gradient.angle = angle;
}

void PresentationEffects::setStroke(bool enabled, float width, ImVec4 stroke_color, ImVec4 fill_color) {
    stroke.enabled = enabled;
    stroke.width = width;
    stroke.stroke_color = stroke_color;
    stroke.color = fill_color;
}

void PresentationEffects::setTextBackground(bool enabled, ImVec4 color, float padding_x, float padding_y, float corner_radius) {
    background.enabled = enabled;
    background.color = color;
    background.padding_x = padding_x;
    background.padding_y = padding_y;
    background.corner_radius = corner_radius;
}

// Main rendering methods
void PresentationEffects::beginTextEffects(const ImVec2& position, const ImVec2& size, const std::string& text, 
                                         ImFont* font, float font_size) {
    current_draw_list = ImGui::GetWindowDrawList();
    current_position = position;
    current_size = size;
    current_text = text;
    current_font = font ? font : ImGui::GetFont();
    current_font_size = font_size > 0 ? font_size : ImGui::GetFontSize();
    
    // Render background first
    if (background.enabled) {
        renderTextBackground(position, size);
    }
    
    // Render effects in order (back to front)
    if (drop_shadow.enabled) {
        renderDropShadow(position, size, text, current_font, current_font_size);
    }
    
    if (glow.enabled) {
        renderGlow(position, size, text, current_font, current_font_size);
    }
    
    if (outline.enabled) {
        renderOutline(position, size, text, current_font, current_font_size);
    }
    
    if (stroke.enabled) {
        renderStrokedText(position, size, text, current_font, current_font_size);
    } else if (gradient.enabled) {
        renderGradientText(position, size, text, current_font, current_font_size);
    } else {
        // Render normal text as fallback
        ImGui::PushFont(current_font);
        ImU32 text_color = IM_COL32(255, 255, 255, 255);
        current_draw_list->AddText(current_font, current_font_size, position, text_color, text.c_str());
        ImGui::PopFont();
    }
}

void PresentationEffects::endTextEffects() {
    current_draw_list = nullptr;
}

// Individual effect rendering methods
void PresentationEffects::renderDropShadow(const ImVec2& position, const ImVec2& size, const std::string& text, 
                                         ImFont* font, float font_size) {
    if (!current_draw_list || !drop_shadow.enabled) return;
    
    ImVec2 shadow_pos = ImVec2(position.x + drop_shadow.offset_x, position.y + drop_shadow.offset_y);
    ImU32 shadow_color = imVec4ToImU32(drop_shadow.color);
    
    // For blur effect, render multiple times with slight offsets
    if (drop_shadow.blur_radius > 0) {
        int samples = std::max(1, static_cast<int>(drop_shadow.blur_radius / 2));
        float alpha_step = drop_shadow.color.w / samples;
        
        for (int i = 0; i < samples; i++) {
            float offset = static_cast<float>(i) * 0.5f;
            ImVec2 blur_pos = ImVec2(shadow_pos.x + offset, shadow_pos.y + offset);
            ImVec4 blur_color = drop_shadow.color;
            blur_color.w = alpha_step;
            
            ImGui::PushFont(font);
            current_draw_list->AddText(font, font_size, blur_pos, imVec4ToImU32(blur_color), text.c_str());
            ImGui::PopFont();
        }
    } else {
        ImGui::PushFont(font);
        current_draw_list->AddText(font, font_size, shadow_pos, shadow_color, text.c_str());
        ImGui::PopFont();
    }
}

void PresentationEffects::renderOutline(const ImVec2& position, const ImVec2& size, const std::string& text, 
                                      ImFont* font, float font_size) {
    if (!current_draw_list || !outline.enabled) return;
    
    ImU32 outline_color = imVec4ToImU32(outline.color);
    int thickness = static_cast<int>(outline.thickness);
    
    // Render text multiple times in all directions for outline effect
    for (int dx = -thickness; dx <= thickness; dx++) {
        for (int dy = -thickness; dy <= thickness; dy++) {
            if (dx == 0 && dy == 0) continue; // Skip center (will be rendered as main text)
            
            ImVec2 outline_pos = ImVec2(position.x + dx, position.y + dy);
            ImGui::PushFont(font);
            current_draw_list->AddText(font, font_size, outline_pos, outline_color, text.c_str());
            ImGui::PopFont();
        }
    }
}

void PresentationEffects::renderGlow(const ImVec2& position, const ImVec2& size, const std::string& text, 
                                   ImFont* font, float font_size) {
    if (!current_draw_list || !glow.enabled) return;
    
    int samples = static_cast<int>(glow.radius);
    float alpha_step = (glow.color.w * glow.strength) / samples;
    
    for (int i = 1; i <= samples; i++) {
        float radius = static_cast<float>(i);
        ImVec4 glow_color = glow.color;
        glow_color.w = alpha_step * (samples - i + 1) / samples; // Fade out as radius increases
        
        // Render in multiple directions for glow effect
        renderTextMultiple(position, text, font, font_size, imVec4ToImU32(glow_color), 
                          static_cast<int>(radius), static_cast<int>(radius), 8);
    }
}

void PresentationEffects::renderGradientText(const ImVec2& position, const ImVec2& size, const std::string& text, 
                                           ImFont* font, float font_size) {
    if (!current_draw_list || !gradient.enabled) return;
    
    // For simplicity, render as interpolated color based on vertical position
    // More advanced gradient rendering would require custom shaders
    float progress = (gradient.angle == 90.0f) ? 0.5f : 0.5f; // Middle gradient for now
    ImVec4 grad_color = interpolateGradient(gradient.start_color, gradient.end_color, progress, gradient.angle);
    
    ImGui::PushFont(font);
    current_draw_list->AddText(font, font_size, position, imVec4ToImU32(grad_color), text.c_str());
    ImGui::PopFont();
}

void PresentationEffects::renderStrokedText(const ImVec2& position, const ImVec2& size, const std::string& text, 
                                          ImFont* font, float font_size) {
    if (!current_draw_list || !stroke.enabled) return;
    
    // Render stroke (outline)
    ImU32 stroke_color = imVec4ToImU32(stroke.stroke_color);
    int width = static_cast<int>(stroke.width);
    
    for (int dx = -width; dx <= width; dx++) {
        for (int dy = -width; dy <= width; dy++) {
            if (dx == 0 && dy == 0) continue;
            
            ImVec2 stroke_pos = ImVec2(position.x + dx, position.y + dy);
            ImGui::PushFont(font);
            current_draw_list->AddText(font, font_size, stroke_pos, stroke_color, text.c_str());
            ImGui::PopFont();
        }
    }
    
    // Render fill
    ImGui::PushFont(font);
    current_draw_list->AddText(font, font_size, position, imVec4ToImU32(stroke.color), text.c_str());
    ImGui::PopFont();
}

void PresentationEffects::renderTextBackground(const ImVec2& position, const ImVec2& size) {
    if (!current_draw_list || !background.enabled) return;
    
    ImVec2 bg_min = ImVec2(position.x - background.padding_x, position.y - background.padding_y);
    ImVec2 bg_max = ImVec2(position.x + size.x + background.padding_x, position.y + size.y + background.padding_y);
    
    if (background.corner_radius > 0) {
        current_draw_list->AddRectFilled(bg_min, bg_max, imVec4ToImU32(background.color), background.corner_radius);
    } else {
        current_draw_list->AddRectFilled(bg_min, bg_max, imVec4ToImU32(background.color));
    }
}

// Configuration and presets
void PresentationEffects::resetEffects() {
    drop_shadow = DropShadowEffect();
    outline = OutlineEffect();
    glow = GlowEffect();
    gradient = GradientEffect();
    stroke = StrokeEffect();
    background = BackgroundEffect();
}

void PresentationEffects::loadPreset(const std::string& preset_name) {
    resetEffects();
    
    if (preset_name == "classic") {
        setDropShadow(true, 2.0f, 2.0f, 4.0f, ImVec4(0.0f, 0.0f, 0.0f, 0.8f));
    } else if (preset_name == "modern") {
        setOutline(true, 1.0f, ImVec4(0.0f, 0.0f, 0.0f, 0.9f));
        setGlow(true, 5.0f, 0.3f, ImVec4(1.0f, 1.0f, 1.0f, 0.6f));
    } else if (preset_name == "bold") {
        setStroke(true, 3.0f, ImVec4(0.0f, 0.0f, 0.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        setTextBackground(true, ImVec4(0.0f, 0.0f, 0.0f, 0.7f), 15.0f, 10.0f, 8.0f);
    } else if (preset_name == "elegant") {
        setGradient(true, ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec4(0.9f, 0.9f, 0.9f, 1.0f), 90.0f);
        setDropShadow(true, 1.0f, 1.0f, 2.0f, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
    }
}

void PresentationEffects::savePreset(const std::string& preset_name) {
    // TODO: Implement preset saving to file
    // For now, this is a placeholder for future implementation
}

// Utility methods
ImVec4 PresentationEffects::blendColors(ImVec4 color1, ImVec4 color2, float factor) {
    factor = std::clamp(factor, 0.0f, 1.0f);
    return ImVec4(
        color1.x + (color2.x - color1.x) * factor,
        color1.y + (color2.y - color1.y) * factor,
        color1.z + (color2.z - color1.z) * factor,
        color1.w + (color2.w - color1.w) * factor
    );
}

ImVec4 PresentationEffects::interpolateGradient(ImVec4 start, ImVec4 end, float position, float angle) {
    // Simplified gradient interpolation
    float factor = std::clamp(position, 0.0f, 1.0f);
    
    // Adjust factor based on angle (simplified)
    if (angle == 0.0f) { // Horizontal
        // Use position as-is
    } else if (angle == 90.0f) { // Vertical
        // Use position as-is
    } else {
        // For other angles, use simplified calculation
        factor = (std::sin(angle * M_PI / 180.0f) + 1.0f) * 0.5f;
    }
    
    return blendColors(start, end, factor);
}

void PresentationEffects::drawBlurredRect(ImDrawList* draw_list, const ImVec2& min, const ImVec2& max, 
                                        ImU32 color, float blur_radius, float corner_radius) {
    if (!draw_list) return;
    
    if (blur_radius <= 0) {
        if (corner_radius > 0) {
            draw_list->AddRectFilled(min, max, color, corner_radius);
        } else {
            draw_list->AddRectFilled(min, max, color);
        }
        return;
    }
    
    // Simple blur approximation by drawing multiple slightly offset rectangles
    int samples = static_cast<int>(blur_radius);
    ImVec4 color_vec = ImGui::ColorConvertU32ToFloat4(color);
    color_vec.w /= samples; // Reduce alpha for each sample
    
    for (int i = 0; i < samples; i++) {
        float offset = static_cast<float>(i) * 0.5f;
        ImVec2 blur_min = ImVec2(min.x - offset, min.y - offset);
        ImVec2 blur_max = ImVec2(max.x + offset, max.y + offset);
        
        ImU32 blur_color = IM_COL32(
            static_cast<int>(color_vec.x * 255),
            static_cast<int>(color_vec.y * 255),
            static_cast<int>(color_vec.z * 255),
            static_cast<int>(color_vec.w * 255)
        );
        
        if (corner_radius > 0) {
            draw_list->AddRectFilled(blur_min, blur_max, blur_color, corner_radius);
        } else {
            draw_list->AddRectFilled(blur_min, blur_max, blur_color);
        }
    }
}

// Private helper methods
void PresentationEffects::renderTextWithColor(const ImVec2& position, const std::string& text, 
                                            ImFont* font, float font_size, ImU32 color) {
    if (!current_draw_list) return;
    
    ImGui::PushFont(font);
    current_draw_list->AddText(font, font_size, position, color, text.c_str());
    ImGui::PopFont();
}

void PresentationEffects::renderTextMultiple(const ImVec2& position, const std::string& text, 
                                           ImFont* font, float font_size, ImU32 color, 
                                           int offset_x, int offset_y, int samples) {
    if (!current_draw_list) return;
    
    float angle_step = 2.0f * M_PI / samples;
    
    for (int i = 0; i < samples; i++) {
        float angle = i * angle_step;
        float dx = std::cos(angle) * offset_x;
        float dy = std::sin(angle) * offset_y;
        
        ImVec2 sample_pos = ImVec2(position.x + dx, position.y + dy);
        renderTextWithColor(sample_pos, text, font, font_size, color);
    }
}

ImU32 PresentationEffects::imVec4ToImU32(const ImVec4& color) {
    return IM_COL32(
        static_cast<int>(color.x * 255),
        static_cast<int>(color.y * 255),
        static_cast<int>(color.z * 255),
        static_cast<int>(color.w * 255)
    );
}

float PresentationEffects::calculateFontSize(ImFont* font, float desired_size) {
    if (!font) return desired_size;
    return desired_size; // Simplified - could be enhanced with font scaling logic
}