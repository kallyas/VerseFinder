#include "MediaManager.h"
#include <imgui.h>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <random>

MediaManager::MediaManager() 
    : video_playing(false), video_position(0.0f), camera_active(false),
      camera_width(1280), camera_height(720), camera_fps(30.0f),
      current_background_texture(0), weather_enabled(false) {
    
    // Initialize with default background
    current_background.type = BackgroundType::SOLID_COLOR;
    current_background.colors.clear();
    current_background.colors.push_back(0xFF000000); // Black
}

MediaManager::~MediaManager() {
    clearAllAssets();
    shutdownCamera();
}

// Asset management
bool MediaManager::loadMediaAsset(const std::string& file_path, const std::string& name, 
                                const std::vector<std::string>& tags) {
    if (!std::filesystem::exists(file_path)) {
        return false;
    }
    
    std::string asset_id = generateAssetId(file_path);
    
    // Check if already loaded
    if (media_assets.find(asset_id) != media_assets.end()) {
        return true; // Already loaded
    }
    
    MediaAsset asset;
    asset.id = asset_id;
    asset.name = name.empty() ? std::filesystem::path(file_path).stem().string() : name;
    asset.file_path = file_path;
    asset.type = detectMediaType(file_path);
    asset.tags = tags;
    asset.file_size = getFileSize(file_path);
    asset.last_used = std::chrono::system_clock::now();
    asset.loaded = false;
    
    // Load the actual asset data based on type
    bool load_success = false;
    switch (asset.type) {
        case MediaType::IMAGE:
            load_success = loadImageAsset(asset);
            break;
        case MediaType::VIDEO:
            load_success = loadVideoAsset(asset);
            break;
        case MediaType::AUDIO:
            // Audio loading to be implemented
            load_success = true;
            break;
        default:
            load_success = false;
            break;
    }
    
    if (load_success) {
        media_assets[asset_id] = asset;
        return true;
    }
    
    return false;
}

bool MediaManager::unloadMediaAsset(const std::string& media_id) {
    auto it = media_assets.find(media_id);
    if (it != media_assets.end()) {
        // TODO: Release GPU resources if any
        media_assets.erase(it);
        return true;
    }
    return false;
}

void MediaManager::clearAllAssets() {
    media_assets.clear();
    current_background_texture = 0;
}

// Asset discovery
void MediaManager::scanDirectory(const std::string& directory_path, bool recursive) {
    if (!std::filesystem::exists(directory_path)) {
        return;
    }
    
    auto scan_files = [this](const std::filesystem::path& dir_path) {
        try {
            for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
                if (entry.is_regular_file()) {
                    std::string file_path = entry.path().string();
                    if (isFormatSupported(file_path)) {
                        // Auto-tag based on directory structure
                        std::vector<std::string> tags;
                        std::string parent_dir = entry.path().parent_path().filename().string();
                        if (!parent_dir.empty()) {
                            tags.push_back(parent_dir);
                        }
                        
                        loadMediaAsset(file_path, "", tags);
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            // Handle filesystem errors silently
        }
    };
    
    if (recursive) {
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(directory_path)) {
                if (entry.is_directory()) {
                    scan_files(entry.path());
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            // Handle filesystem errors silently
        }
    } else {
        scan_files(directory_path);
    }
}

void MediaManager::scanForSeasonalContent() {
    // Look for seasonal content in common directories
    std::vector<std::string> seasonal_dirs = {
        "media/seasonal", "backgrounds/seasonal", "assets/seasonal",
        "media/christmas", "media/easter", "media/thanksgiving"
    };
    
    for (const auto& dir : seasonal_dirs) {
        if (std::filesystem::exists(dir)) {
            scanDirectory(dir, true);
        }
    }
}

// Asset querying
std::vector<MediaAsset> MediaManager::getAssetsByType(MediaType type) const {
    std::vector<MediaAsset> result;
    for (const auto& [id, asset] : media_assets) {
        if (asset.type == type) {
            result.push_back(asset);
        }
    }
    return result;
}

std::vector<MediaAsset> MediaManager::getAssetsByTag(const std::string& tag) const {
    std::vector<MediaAsset> result;
    for (const auto& [id, asset] : media_assets) {
        if (std::find(asset.tags.begin(), asset.tags.end(), tag) != asset.tags.end()) {
            result.push_back(asset);
        }
    }
    return result;
}

std::vector<MediaAsset> MediaManager::searchAssets(const std::string& query) const {
    std::vector<MediaAsset> result;
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    
    for (const auto& [id, asset] : media_assets) {
        std::string lower_name = asset.name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        
        if (lower_name.find(lower_query) != std::string::npos) {
            result.push_back(asset);
            continue;
        }
        
        // Search in tags
        for (const auto& tag : asset.tags) {
            std::string lower_tag = tag;
            std::transform(lower_tag.begin(), lower_tag.end(), lower_tag.begin(), ::tolower);
            if (lower_tag.find(lower_query) != std::string::npos) {
                result.push_back(asset);
                break;
            }
        }
    }
    
    return result;
}

MediaAsset* MediaManager::getAsset(const std::string& media_id) {
    auto it = media_assets.find(media_id);
    return (it != media_assets.end()) ? &it->second : nullptr;
}

const MediaAsset* MediaManager::getAsset(const std::string& media_id) const {
    auto it = media_assets.find(media_id);
    return (it != media_assets.end()) ? &it->second : nullptr;
}

// Background management
void MediaManager::setBackground(const BackgroundConfig& config) {
    current_background = config;
    
    // Update background texture if needed
    if (config.type == BackgroundType::IMAGE || config.type == BackgroundType::VIDEO) {
        const MediaAsset* asset = getAsset(config.media_id);
        if (asset && asset->loaded) {
            // TODO: Set current_background_texture from asset
        }
    }
    
    // Trigger callback
    if (background_change_callback) {
        background_change_callback(current_background);
    }
}

void MediaManager::clearBackground() {
    BackgroundConfig default_config;
    setBackground(default_config);
}

// Seasonal themes
void MediaManager::loadSeasonalThemes() {
    seasonal_themes.clear();
    
    // Create default seasonal themes
    SeasonalTheme christmas;
    christmas.name = "Christmas";
    christmas.category = "christmas";
    christmas.active = false;
    // Set Christmas dates (December 1 - December 31)
    std::tm tm_start = {};
    tm_start.tm_year = 2024 - 1900; // Current year - 1900
    tm_start.tm_mon = 11; // December (0-based)
    tm_start.tm_mday = 1;
    christmas.start_date = std::chrono::system_clock::from_time_t(std::mktime(&tm_start));
    
    std::tm tm_end = {};
    tm_end.tm_year = 2024 - 1900;
    tm_end.tm_mon = 11;
    tm_end.tm_mday = 31;
    christmas.end_date = std::chrono::system_clock::from_time_t(std::mktime(&tm_end));
    
    seasonal_themes.push_back(christmas);
    
    // Add more seasonal themes as needed
    SeasonalTheme easter;
    easter.name = "Easter";
    easter.category = "easter";
    easter.active = false;
    seasonal_themes.push_back(easter);
}

void MediaManager::activateSeasonalTheme(const std::string& theme_name) {
    for (auto& theme : seasonal_themes) {
        if (theme.name == theme_name) {
            theme.active = true;
            
            // Apply theme background if available
            if (!theme.media_ids.empty()) {
                BackgroundConfig config;
                config.type = BackgroundType::SEASONAL_THEME;
                config.theme_category = theme.category;
                
                // Select random media from theme
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(0, theme.media_ids.size() - 1);
                config.media_id = theme.media_ids[dis(gen)];
                
                setBackground(config);
            }
            break;
        }
    }
}

void MediaManager::deactivateSeasonalTheme() {
    for (auto& theme : seasonal_themes) {
        theme.active = false;
    }
}

std::vector<SeasonalTheme> MediaManager::getActiveSeasonalThemes() const {
    std::vector<SeasonalTheme> active_themes;
    auto now = std::chrono::system_clock::now();
    
    for (const auto& theme : seasonal_themes) {
        if (theme.active && isDateInRange(now, theme.start_date, theme.end_date)) {
            active_themes.push_back(theme);
        }
    }
    
    return active_themes;
}

bool MediaManager::isSeasonalThemeActive() const {
    return !getActiveSeasonalThemes().empty();
}

// Video background control
void MediaManager::playVideo() {
    video_playing = true;
    video_start_time = std::chrono::steady_clock::now();
}

void MediaManager::pauseVideo() {
    video_playing = false;
}

void MediaManager::stopVideo() {
    video_playing = false;
    video_position = 0.0f;
}

void MediaManager::setVideoPosition(float position) {
    video_position = std::clamp(position, 0.0f, 1.0f);
    video_start_time = std::chrono::steady_clock::now();
}

float MediaManager::getVideoDuration() const {
    if (current_background.type == BackgroundType::VIDEO) {
        const MediaAsset* asset = getAsset(current_background.media_id);
        if (asset) {
            return asset->duration;
        }
    }
    return 0.0f;
}

// Live camera integration
bool MediaManager::initializeCamera(int camera_index) {
    // TODO: Implement camera initialization
    // This would involve platform-specific camera APIs
    camera_active = true;
    return true;
}

void MediaManager::shutdownCamera() {
    // TODO: Implement camera shutdown
    camera_active = false;
}

void MediaManager::setCameraSettings(int width, int height, float fps) {
    camera_width = width;
    camera_height = height;
    camera_fps = fps;
}

// Rendering interface
void MediaManager::renderCurrentBackground(const ImVec2& position, const ImVec2& size) {
    switch (current_background.type) {
        case BackgroundType::SOLID_COLOR:
            renderSolidBackground(position, size);
            break;
        case BackgroundType::GRADIENT:
            renderGradientBackground(position, size);
            break;
        case BackgroundType::IMAGE:
        case BackgroundType::SEASONAL_THEME:
            renderImageBackground(position, size);
            break;
        case BackgroundType::VIDEO:
            renderVideoBackground(position, size);
            updateVideoPlayback();
            break;
        case BackgroundType::LIVE_CAMERA:
            renderCameraBackground(position, size);
            break;
        case BackgroundType::DYNAMIC_WEATHER:
            if (weather_enabled) {
                updateWeatherBackground();
                renderImageBackground(position, size);
            } else {
                renderSolidBackground(position, size);
            }
            break;
    }
}

// Static utility methods
std::vector<std::string> MediaManager::getSupportedImageFormats() {
    return {".jpg", ".jpeg", ".png", ".bmp", ".tga", ".gif"};
}

std::vector<std::string> MediaManager::getSupportedVideoFormats() {
    return {".mp4", ".avi", ".mov", ".mkv", ".wmv", ".webm"};
}

bool MediaManager::isFormatSupported(const std::string& file_path) {
    std::string ext = std::filesystem::path(file_path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    auto image_formats = getSupportedImageFormats();
    auto video_formats = getSupportedVideoFormats();
    
    return std::find(image_formats.begin(), image_formats.end(), ext) != image_formats.end() ||
           std::find(video_formats.begin(), video_formats.end(), ext) != video_formats.end();
}

// Private helper methods
std::string MediaManager::generateAssetId(const std::string& file_path) const {
    // Simple hash-based ID generation
    std::hash<std::string> hasher;
    return std::to_string(hasher(file_path));
}

MediaType MediaManager::detectMediaType(const std::string& file_path) const {
    std::string ext = std::filesystem::path(file_path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (isImageFile(file_path)) return MediaType::IMAGE;
    if (isVideoFile(file_path)) return MediaType::VIDEO;
    
    std::vector<std::string> audio_formats = {".mp3", ".wav", ".ogg", ".flac", ".aac"};
    if (std::find(audio_formats.begin(), audio_formats.end(), ext) != audio_formats.end()) {
        return MediaType::AUDIO;
    }
    
    return MediaType::UNKNOWN;
}

bool MediaManager::loadImageAsset(MediaAsset& asset) {
    // TODO: Implement actual image loading using stb_image or similar
    // For now, just mark as loaded
    asset.loaded = true;
    asset.width = 1920; // Placeholder
    asset.height = 1080; // Placeholder
    return true;
}

bool MediaManager::loadVideoAsset(MediaAsset& asset) {
    // TODO: Implement actual video loading
    // For now, just mark as loaded
    asset.loaded = true;
    asset.width = 1920; // Placeholder
    asset.height = 1080; // Placeholder
    asset.duration = 60.0f; // Placeholder duration
    return true;
}

void MediaManager::updateVideoPlayback() {
    if (!video_playing) return;
    
    float duration = getVideoDuration();
    if (duration <= 0) return;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - video_start_time).count();
    video_position = static_cast<float>(elapsed) / (duration * 1000.0f);
    
    if (video_position >= 1.0f) {
        if (current_background.loop_video) {
            video_position = 0.0f;
            video_start_time = now;
        } else {
            video_playing = false;
            video_position = 1.0f;
        }
    }
}

// Background rendering methods
void MediaManager::renderSolidBackground(const ImVec2& position, const ImVec2& size) {
    if (current_background.colors.empty()) return;
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImU32 color = current_background.colors[0];
    draw_list->AddRectFilled(position, ImVec2(position.x + size.x, position.y + size.y), color);
}

void MediaManager::renderGradientBackground(const ImVec2& position, const ImVec2& size) {
    if (current_background.colors.size() < 2) {
        renderSolidBackground(position, size);
        return;
    }
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImU32 color1 = current_background.colors[0];
    ImU32 color2 = current_background.colors[1];
    
    // Simple gradient implementation
    bool horizontal = (current_background.gradient_angle == 0.0f || current_background.gradient_angle == 180.0f);
    
    if (horizontal) {
        draw_list->AddRectFilledMultiColor(
            position, ImVec2(position.x + size.x, position.y + size.y),
            color1, color2, color2, color1
        );
    } else {
        draw_list->AddRectFilledMultiColor(
            position, ImVec2(position.x + size.x, position.y + size.y),
            color1, color1, color2, color2
        );
    }
}

void MediaManager::renderImageBackground(const ImVec2& position, const ImVec2& size) {
    // TODO: Implement actual image rendering
    // For now, render a placeholder
    renderSolidBackground(position, size);
}

void MediaManager::renderVideoBackground(const ImVec2& position, const ImVec2& size) {
    // TODO: Implement actual video frame rendering
    // For now, render a placeholder with position indicator
    renderSolidBackground(position, size);
    
    // Draw a simple progress indicator
    if (video_playing) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        float progress_width = size.x * video_position;
        ImVec2 progress_end = ImVec2(position.x + progress_width, position.y + 5);
        draw_list->AddRectFilled(position, progress_end, IM_COL32(255, 255, 255, 128));
    }
}

void MediaManager::renderCameraBackground(const ImVec2& position, const ImVec2& size) {
    // TODO: Implement camera frame rendering
    // For now, render a placeholder
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(position, ImVec2(position.x + size.x, position.y + size.y), 
                           IM_COL32(64, 64, 128, 255)); // Blue placeholder
}

// Weather and seasonal helpers
void MediaManager::enableWeatherBasedBackground(const std::string& location) {
    weather_enabled = true;
    weather_location = location;
    updateWeatherBackground();
}

void MediaManager::disableWeatherBasedBackground() {
    weather_enabled = false;
}

void MediaManager::updateWeatherBackground() {
    if (!weather_enabled) return;
    
    auto now = std::chrono::steady_clock::now();
    auto duration_since_update = std::chrono::duration_cast<std::chrono::minutes>(now - last_weather_update);
    
    // Update weather every 30 minutes
    if (duration_since_update.count() >= 30) {
        std::string condition = getCurrentWeatherCondition();
        BackgroundConfig weather_bg = getWeatherBackground(condition);
        if (!weather_bg.media_id.empty()) {
            setBackground(weather_bg);
        }
        last_weather_update = now;
    }
}

std::string MediaManager::getCurrentWeatherCondition() const {
    // TODO: Implement actual weather API integration
    // For now, return a placeholder
    std::vector<std::string> conditions = {"sunny", "cloudy", "rainy", "snowy"};
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, conditions.size() - 1);
    return conditions[dis(gen)];
}

BackgroundConfig MediaManager::getWeatherBackground(const std::string& condition) const {
    BackgroundConfig config;
    config.type = BackgroundType::DYNAMIC_WEATHER;
    
    // Find weather-appropriate background
    std::vector<MediaAsset> weather_assets = getAssetsByTag(condition);
    if (!weather_assets.empty()) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, weather_assets.size() - 1);
        config.media_id = weather_assets[dis(gen)].id;
    }
    
    return config;
}

bool MediaManager::isDateInRange(const std::chrono::system_clock::time_point& date,
                                const std::chrono::system_clock::time_point& start,
                                const std::chrono::system_clock::time_point& end) const {
    return date >= start && date <= end;
}

bool MediaManager::isImageFile(const std::string& file_path) const {
    std::string ext = std::filesystem::path(file_path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    auto formats = getSupportedImageFormats();
    return std::find(formats.begin(), formats.end(), ext) != formats.end();
}

bool MediaManager::isVideoFile(const std::string& file_path) const {
    std::string ext = std::filesystem::path(file_path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    auto formats = getSupportedVideoFormats();
    return std::find(formats.begin(), formats.end(), ext) != formats.end();
}

size_t MediaManager::getFileSize(const std::string& file_path) const {
    try {
        return std::filesystem::file_size(file_path);
    } catch (const std::filesystem::filesystem_error&) {
        return 0;
    }
}