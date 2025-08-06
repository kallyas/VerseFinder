#ifndef MOBILE_API_H
#define MOBILE_API_H

#include "ApiServer.h"
#include "WebSocketServer.h"
#include "../core/VerseFinder.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

// Forward declaration to avoid circular include
class VerseFinderApp;

struct DevicePairingSession {
    std::string session_id;
    std::string pin_code;
    std::string device_name;
    std::chrono::steady_clock::time_point created_at;
    bool used = false;
};

struct MobileDevice {
    std::string device_id;
    std::string device_name;
    std::string user_id;
    std::string last_ip;
    std::chrono::steady_clock::time_point last_seen;
    bool is_authorized = true;
    std::string permission_level = "user"; // "admin", "presenter", "user"
};

struct PresentationState {
    bool is_active = false;
    std::string current_verse_text;
    std::string current_reference;
    bool is_blank = false;
    bool is_logo_displayed = false;
    std::string background_theme = "default";
    float text_size = 1.0f;
    std::string text_position = "center";
};

class MobileAPI {
public:
    MobileAPI(VerseFinder* bible, VerseFinderApp* app);
    ~MobileAPI();
    
    // Setup API routes on the given servers
    void setupApiRoutes(ApiServer* api_server);
    void setupWebSocketHandlers(WebSocketServer* ws_server);
    
    // Device pairing and authentication
    std::string createPairingSession(const std::string& device_name);
    bool validatePairingPin(const std::string& session_id, const std::string& pin);
    std::string generateAuthToken(const std::string& device_id, const std::string& user_id);
    bool validateAuthToken(const std::string& token, std::string& device_id, std::string& user_id);
    
    // Device management
    void registerDevice(const MobileDevice& device);
    std::vector<MobileDevice> getAuthorizedDevices() const;
    void revokeDeviceAccess(const std::string& device_id);
    void updateDevicePermissions(const std::string& device_id, const std::string& permission_level);
    
    // Presentation control
    PresentationState getCurrentPresentationState() const;
    bool togglePresentationMode();
    bool displayVerse(const std::string& verse_text, const std::string& reference);
    bool toggleBlankScreen();
    bool showLogo();
    bool navigateVerse(int direction); // -1 for previous, +1 for next
    bool setBackgroundTheme(const std::string& theme);
    bool adjustTextSize(float size_multiplier);
    bool setTextPosition(const std::string& position);
    
    // Search and verse retrieval
    std::vector<std::string> searchVerses(const std::string& query, const std::string& translation) const;
    std::string getVerseText(const std::string& reference, const std::string& translation) const;
    std::vector<std::string> getAvailableTranslations() const;
    
    // Favorites and bookmarks
    void addToFavorites(const std::string& verse_reference, const std::string& user_id);
    void removeFromFavorites(const std::string& verse_reference, const std::string& user_id);
    std::vector<std::string> getUserFavorites(const std::string& user_id) const;
    
    // Real-time notifications
    void notifyPresentationStateChange();
    void notifyVerseChange(const std::string& verse_text, const std::string& reference);
    void notifyDeviceConnected(const std::string& device_name);
    void notifyDeviceDisconnected(const std::string& device_name);
    
    // Settings and configuration
    void updateMobileSettings(const std::string& settings_json);
    std::string getMobileSettings() const;
    
    // Emergency and quick access
    std::vector<std::string> getEmergencyVerses() const;
    std::vector<std::string> getQuickAccessVerses() const;
    void addQuickAccessVerse(const std::string& verse_reference);
    void removeQuickAccessVerse(const std::string& verse_reference);

private:
    VerseFinder* bible_;
    VerseFinderApp* app_;
    WebSocketServer* ws_server_;
    
    // Session and device management
    std::unordered_map<std::string, DevicePairingSession> pairing_sessions_;
    std::unordered_map<std::string, MobileDevice> authorized_devices_;
    std::unordered_map<std::string, std::string> auth_tokens_; // token -> device_id
    
    // User data storage
    std::unordered_map<std::string, std::vector<std::string>> user_favorites_;
    std::vector<std::string> quick_access_verses_;
    std::string mobile_settings_json_;
    
    // Default emergency verses
    std::vector<std::string> emergency_verses_ = {
        "Psalm 23:1-6",
        "John 3:16",
        "Romans 8:28",
        "Philippians 4:13",
        "Isaiah 41:10",
        "2 Corinthians 5:17",
        "1 John 1:9",
        "Matthew 11:28-30"
    };
    
    // API endpoint handlers
    ApiResponse handlePairingRequest(const ApiRequest& request);
    ApiResponse handlePairingValidation(const ApiRequest& request);
    ApiResponse handleAuthTokenRequest(const ApiRequest& request);
    ApiResponse handleDeviceStatus(const ApiRequest& request);
    ApiResponse handlePresentationControl(const ApiRequest& request);
    ApiResponse handleVerseSearch(const ApiRequest& request);
    ApiResponse handleVerseDisplay(const ApiRequest& request);
    ApiResponse handleFavorites(const ApiRequest& request);
    ApiResponse handleQuickAccess(const ApiRequest& request);
    ApiResponse handleEmergency(const ApiRequest& request);
    ApiResponse handleSettings(const ApiRequest& request);
    
    // WebSocket message handlers
    void handleWebSocketAuth(const std::string& connection_id, const WebSocketMessage& message);
    void handlePresentationCommand(const std::string& connection_id, const WebSocketMessage& message);
    void handleSearchRequest(const std::string& connection_id, const WebSocketMessage& message);
    void handleSubscribeEvents(const std::string& connection_id, const WebSocketMessage& message);
    void handleHeartbeat(const std::string& connection_id, const WebSocketMessage& message);
    
    // Utility methods
    std::string generatePinCode();
    std::string generateSessionId();
    std::string generateAuthToken();
    bool hasPermission(const std::string& device_id, const std::string& required_permission) const;
    void cleanupExpiredSessions();
    std::string deviceToJson(const MobileDevice& device) const;
    std::string presentationStateToJson(const PresentationState& state) const;
    std::string parseJsonString(const std::string& json, const std::string& key) const;
    int parseJsonInt(const std::string& json, const std::string& key) const;
    bool parseJsonBool(const std::string& json, const std::string& key) const;
};

#endif // MOBILE_API_H