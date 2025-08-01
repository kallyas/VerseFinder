#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <imgui.h>
#include <string>
#include <vector>

class ThemeManager {
public:
    ThemeManager();
    ~ThemeManager();

    // Theme application
    void setupImGuiStyle(const std::string& theme_name, float font_scale = 1.0f);
    
    // Individual theme methods
    void applyDarkTheme();
    void applyLightTheme();
    void applyBlueTheme();
    void applyGreenTheme();
    
    // Theme utilities
    static std::vector<std::string> getAvailableThemes();
    bool isValidTheme(const std::string& theme_name);

private:
    void applyModernStyling();
    void setCommonColors(ImVec4* colors);
};

#endif // THEME_MANAGER_H