#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include <imgui.h>
#include <string>

class FontManager {
public:
    FontManager();
    ~FontManager();

    bool initializeFonts();
    float getSystemFontSize() const;
    ImFont* getRegularFont() const { return regular_font; }

private:
    std::string getExecutablePath() const;
    bool loadRegularFont();
    
    ImFont* regular_font;
    float system_font_size;
};

#endif // FONT_MANAGER_H