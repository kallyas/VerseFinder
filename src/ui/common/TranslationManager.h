#ifndef TRANSLATION_MANAGER_H
#define TRANSLATION_MANAGER_H

#include <string>

struct DownloadableTranslation {
    std::string name;
    std::string abbreviation;
    std::string url;
    std::string description;
    bool is_downloaded;
    bool is_downloading;
    float download_progress;
    
    DownloadableTranslation(const std::string& n, const std::string& abbr, 
                          const std::string& u, const std::string& desc,
                          bool downloaded = false, bool downloading = false, 
                          float progress = 0.0f)
        : name(n), abbreviation(abbr), url(u), description(desc)
        , is_downloaded(downloaded), is_downloading(downloading)
        , download_progress(progress) {}
};

#endif // TRANSLATION_MANAGER_H