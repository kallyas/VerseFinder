#ifndef MEDIA_MANAGER_H
#define MEDIA_MANAGER_H

#include <imgui.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <functional>

enum class MediaType {
    IMAGE,
    VIDEO,
    AUDIO,
    UNKNOWN
};

enum class BackgroundType {
    SOLID_COLOR,
    GRADIENT,
    IMAGE,
    VIDEO,
    LIVE_CAMERA,
    SEASONAL_THEME,
    DYNAMIC_WEATHER
};

struct MediaAsset {
    std::string id;
    std::string name;
    std::string file_path;
    MediaType type;
    std::string description;
    std::vector<std::string> tags;
    std::chrono::system_clock::time_point last_used;
    bool loaded;
    size_t file_size;
    int width;
    int height;
    float duration; // For video/audio
    
    MediaAsset() : type(MediaType::UNKNOWN), loaded(false), file_size(0), 
                   width(0), height(0), duration(0.0f) {}
};

struct BackgroundConfig {
    BackgroundType type;
    std::string media_id; // For image/video backgrounds
    
    // Color/Gradient settings
    std::vector<uint32_t> colors; // ImU32 colors
    float gradient_angle;
    
    // Video settings
    bool loop_video;
    float video_opacity;
    bool muted;
    
    // Ken Burns effect
    bool ken_burns_enabled;
    float ken_burns_duration;
    float zoom_start;
    float zoom_end;
    float pan_x;
    float pan_y;
    
    // Dynamic settings
    std::string theme_category;
    std::string weather_location;
    bool auto_change;
    float change_interval; // minutes
    
    BackgroundConfig() : type(BackgroundType::SOLID_COLOR), gradient_angle(90.0f), 
                        loop_video(true), video_opacity(1.0f), muted(true),
                        ken_burns_enabled(false), ken_burns_duration(10.0f),
                        zoom_start(1.0f), zoom_end(1.1f), pan_x(0.0f), pan_y(0.0f),
                        auto_change(false), change_interval(5.0f) {
        colors.push_back(0xFF000000); // Default black
    }
};

struct SeasonalTheme {
    std::string name;
    std::string category; // "christmas", "easter", "summer", etc.
    std::vector<std::string> media_ids;
    std::chrono::system_clock::time_point start_date;
    std::chrono::system_clock::time_point end_date;
    bool active;
    
    SeasonalTheme() : active(false) {}
};

class MediaManager {
public:
    MediaManager();
    ~MediaManager();
    
    // Asset management
    bool loadMediaAsset(const std::string& file_path, const std::string& name = "", 
                       const std::vector<std::string>& tags = {});
    bool unloadMediaAsset(const std::string& media_id);
    void clearAllAssets();
    
    // Asset discovery
    void scanDirectory(const std::string& directory_path, bool recursive = true);
    void scanForSeasonalContent();
    
    // Asset querying
    std::vector<MediaAsset> getAssetsByType(MediaType type) const;
    std::vector<MediaAsset> getAssetsByTag(const std::string& tag) const;
    std::vector<MediaAsset> searchAssets(const std::string& query) const;
    MediaAsset* getAsset(const std::string& media_id);
    const MediaAsset* getAsset(const std::string& media_id) const;
    
    // Background management
    void setBackground(const BackgroundConfig& config);
    const BackgroundConfig& getCurrentBackground() const { return current_background; }
    void clearBackground();
    
    // Seasonal themes
    void loadSeasonalThemes();
    void activateSeasonalTheme(const std::string& theme_name);
    void deactivateSeasonalTheme();
    std::vector<SeasonalTheme> getActiveSeasonalThemes() const;
    bool isSeasonalThemeActive() const;
    
    // Dynamic backgrounds
    void enableWeatherBasedBackground(const std::string& location);
    void disableWeatherBasedBackground();
    void updateWeatherBackground();
    
    // Video background control
    void playVideo();
    void pauseVideo();
    void stopVideo();
    void setVideoPosition(float position); // 0.0 to 1.0
    bool isVideoPlaying() const { return video_playing; }
    float getVideoPosition() const { return video_position; }
    float getVideoDuration() const;
    
    // Live camera integration
    bool initializeCamera(int camera_index = 0);
    void shutdownCamera();
    bool isCameraActive() const { return camera_active; }
    void setCameraSettings(int width, int height, float fps);
    
    // Template and branding
    void loadChurchBrandingTemplates();
    void applyBrandingTemplate(const std::string& template_name);
    std::vector<std::string> getAvailableTemplates() const;
    
    // Asset optimization
    void preloadAssets(const std::vector<std::string>& asset_ids);
    void unloadUnusedAssets(std::chrono::minutes unused_threshold = std::chrono::minutes(30));
    size_t getTotalMemoryUsage() const;
    void optimizeMemoryUsage();
    
    // Rendering interface
    void renderCurrentBackground(const ImVec2& position, const ImVec2& size);
    uint32_t getCurrentBackgroundTexture() const { return current_background_texture; }
    
    // Callbacks for updates
    void setBackgroundChangeCallback(std::function<void(const BackgroundConfig&)> callback) {
        background_change_callback = callback;
    }
    
    // File format support
    static std::vector<std::string> getSupportedImageFormats();
    static std::vector<std::string> getSupportedVideoFormats();
    static bool isFormatSupported(const std::string& file_path);
    
private:
    std::unordered_map<std::string, MediaAsset> media_assets;
    std::vector<SeasonalTheme> seasonal_themes;
    BackgroundConfig current_background;
    
    // Video playback state
    bool video_playing;
    float video_position;
    std::chrono::steady_clock::time_point video_start_time;
    
    // Camera state
    bool camera_active;
    int camera_width;
    int camera_height;
    float camera_fps;
    
    // Rendering
    uint32_t current_background_texture;
    
    // Dynamic background state
    bool weather_enabled;
    std::string weather_location;
    std::chrono::steady_clock::time_point last_weather_update;
    
    // Callbacks
    std::function<void(const BackgroundConfig&)> background_change_callback;
    
    // Helper methods
    std::string generateAssetId(const std::string& file_path) const;
    MediaType detectMediaType(const std::string& file_path) const;
    bool loadImageAsset(MediaAsset& asset);
    bool loadVideoAsset(MediaAsset& asset);
    void updateVideoPlayback();
    void renderSolidBackground(const ImVec2& position, const ImVec2& size);
    void renderGradientBackground(const ImVec2& position, const ImVec2& size);
    void renderImageBackground(const ImVec2& position, const ImVec2& size);
    void renderVideoBackground(const ImVec2& position, const ImVec2& size);
    void renderCameraBackground(const ImVec2& position, const ImVec2& size);
    
    // Seasonal theme helpers
    bool isDateInRange(const std::chrono::system_clock::time_point& date,
                      const std::chrono::system_clock::time_point& start,
                      const std::chrono::system_clock::time_point& end) const;
    void checkSeasonalThemes();
    
    // Weather integration helpers
    std::string getCurrentWeatherCondition() const;
    BackgroundConfig getWeatherBackground(const std::string& condition) const;
    
    // File system helpers
    std::vector<std::string> getFilesInDirectory(const std::string& directory, bool recursive) const;
    bool isImageFile(const std::string& file_path) const;
    bool isVideoFile(const std::string& file_path) const;
    size_t getFileSize(const std::string& file_path) const;
};

#endif // MEDIA_MANAGER_H