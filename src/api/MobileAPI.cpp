#include "MobileAPI.h"
#include "../ui/VerseFinderApp.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>

MobileAPI::MobileAPI(VerseFinder* bible, VerseFinderApp* app) 
    : bible_(bible), app_(app), ws_server_(nullptr) {
    
    // Initialize default mobile settings
    mobile_settings_json_ = R"({
        "theme": "dark",
        "fontSize": "large",
        "vibration": true,
        "sound": true,
        "autoLock": 300,
        "showPreview": true,
        "quickAccessVisible": true
    })";
    
    // Initialize some default quick access verses
    quick_access_verses_ = {
        "John 3:16",
        "Psalm 23:1",
        "Romans 8:28",
        "Philippians 4:13"
    };
}

MobileAPI::~MobileAPI() = default;

void MobileAPI::setupApiRoutes(ApiServer* api_server) {
    // Device pairing endpoints
    api_server->addRoute(HttpMethod::POST, "/api/mobile/pair", 
        [this](const ApiRequest& req) { return handlePairingRequest(req); });
    
    api_server->addRoute(HttpMethod::POST, "/api/mobile/pair/validate", 
        [this](const ApiRequest& req) { return handlePairingValidation(req); });
    
    api_server->addRoute(HttpMethod::POST, "/api/mobile/auth", 
        [this](const ApiRequest& req) { return handleAuthTokenRequest(req); });
    
    // Device management
    api_server->addRoute(HttpMethod::GET, "/api/mobile/devices", 
        [this](const ApiRequest& req) { return handleDeviceStatus(req); });
    
    // Presentation control
    api_server->addRoute(HttpMethod::GET, "/api/mobile/presentation/status", 
        [this](const ApiRequest& req) { return handlePresentationControl(req); });
    
    api_server->addRoute(HttpMethod::POST, "/api/mobile/presentation/toggle", 
        [this](const ApiRequest& req) { return handlePresentationControl(req); });
    
    api_server->addRoute(HttpMethod::POST, "/api/mobile/presentation/blank", 
        [this](const ApiRequest& req) { return handlePresentationControl(req); });
    
    api_server->addRoute(HttpMethod::POST, "/api/mobile/presentation/navigate", 
        [this](const ApiRequest& req) { return handlePresentationControl(req); });
    
    // Verse search and display
    api_server->addRoute(HttpMethod::GET, "/api/mobile/search", 
        [this](const ApiRequest& req) { return handleVerseSearch(req); });
    
    api_server->addRoute(HttpMethod::POST, "/api/mobile/display", 
        [this](const ApiRequest& req) { return handleVerseDisplay(req); });
    
    api_server->addRoute(HttpMethod::GET, "/api/mobile/translations", 
        [this](const ApiRequest& req) { return handleVerseSearch(req); });
    
    // Favorites and bookmarks
    api_server->addRoute(HttpMethod::GET, "/api/mobile/favorites", 
        [this](const ApiRequest& req) { return handleFavorites(req); });
    
    api_server->addRoute(HttpMethod::POST, "/api/mobile/favorites", 
        [this](const ApiRequest& req) { return handleFavorites(req); });
    
    api_server->addRoute(HttpMethod::DELETE, "/api/mobile/favorites", 
        [this](const ApiRequest& req) { return handleFavorites(req); });
    
    // Quick access
    api_server->addRoute(HttpMethod::GET, "/api/mobile/quick-access", 
        [this](const ApiRequest& req) { return handleQuickAccess(req); });
    
    api_server->addRoute(HttpMethod::POST, "/api/mobile/quick-access", 
        [this](const ApiRequest& req) { return handleQuickAccess(req); });
    
    // Emergency access
    api_server->addRoute(HttpMethod::GET, "/api/mobile/emergency", 
        [this](const ApiRequest& req) { return handleEmergency(req); });
    
    // Settings
    api_server->addRoute(HttpMethod::GET, "/api/mobile/settings", 
        [this](const ApiRequest& req) { return handleSettings(req); });
    
    api_server->addRoute(HttpMethod::POST, "/api/mobile/settings", 
        [this](const ApiRequest& req) { return handleSettings(req); });
        
    // Require authentication for all mobile API endpoints except pairing
    api_server->requireAuth("/api/mobile/auth");
    api_server->requireAuth("/api/mobile/devices");
    api_server->requireAuth("/api/mobile/presentation");
    api_server->requireAuth("/api/mobile/search");
    api_server->requireAuth("/api/mobile/display");
    api_server->requireAuth("/api/mobile/translations");
    api_server->requireAuth("/api/mobile/favorites");
    api_server->requireAuth("/api/mobile/quick-access");
    api_server->requireAuth("/api/mobile/emergency");
    api_server->requireAuth("/api/mobile/settings");
}

void MobileAPI::setupWebSocketHandlers(WebSocketServer* ws_server) {
    ws_server_ = ws_server;
    
    // Authentication handler
    ws_server->setMessageHandler("auth", 
        [this](const std::string& conn_id, const WebSocketMessage& msg) {
            handleWebSocketAuth(conn_id, msg);
        });
    
    // Presentation control
    ws_server->setMessageHandler("presentation_command", 
        [this](const std::string& conn_id, const WebSocketMessage& msg) {
            handlePresentationCommand(conn_id, msg);
        });
    
    // Search requests
    ws_server->setMessageHandler("search", 
        [this](const std::string& conn_id, const WebSocketMessage& msg) {
            handleSearchRequest(conn_id, msg);
        });
    
    // Event subscription
    ws_server->setMessageHandler("subscribe", 
        [this](const std::string& conn_id, const WebSocketMessage& msg) {
            handleSubscribeEvents(conn_id, msg);
        });
    
    // Heartbeat
    ws_server->setMessageHandler("heartbeat", 
        [this](const std::string& conn_id, const WebSocketMessage& msg) {
            handleHeartbeat(conn_id, msg);
        });
    
    // Set up authentication handler for WebSocket
    ws_server->setAuthHandler([this](const std::string& token, std::string& user_id) {
        std::string device_id;
        if (validateAuthToken(token, device_id, user_id)) {
            return true;
        }
        return false;
    });
}

std::string MobileAPI::createPairingSession(const std::string& device_name) {
    cleanupExpiredSessions();
    
    DevicePairingSession session;
    session.session_id = generateSessionId();
    session.pin_code = generatePinCode();
    session.device_name = device_name;
    session.created_at = std::chrono::steady_clock::now();
    
    pairing_sessions_[session.session_id] = session;
    
    return session.session_id;
}

bool MobileAPI::validatePairingPin(const std::string& session_id, const std::string& pin) {
    auto it = pairing_sessions_.find(session_id);
    if (it != pairing_sessions_.end() && !it->second.used) {
        if (it->second.pin_code == pin) {
            it->second.used = true;
            return true;
        }
    }
    return false;
}

std::string MobileAPI::generateAuthToken(const std::string& device_id, const std::string& user_id) {
    std::string token = generateAuthToken();
    auth_tokens_[token] = device_id;
    
    // Update device record
    auto it = authorized_devices_.find(device_id);
    if (it != authorized_devices_.end()) {
        it->second.user_id = user_id;
        it->second.last_seen = std::chrono::steady_clock::now();
    }
    
    return token;
}

bool MobileAPI::validateAuthToken(const std::string& token, std::string& device_id, std::string& user_id) {
    auto it = auth_tokens_.find(token);
    if (it != auth_tokens_.end()) {
        device_id = it->second;
        auto device_it = authorized_devices_.find(device_id);
        if (device_it != authorized_devices_.end() && device_it->second.is_authorized) {
            user_id = device_it->second.user_id;
            device_it->second.last_seen = std::chrono::steady_clock::now();
            return true;
        }
    }
    return false;
}

void MobileAPI::registerDevice(const MobileDevice& device) {
    authorized_devices_[device.device_id] = device;
}

std::vector<MobileDevice> MobileAPI::getAuthorizedDevices() const {
    std::vector<MobileDevice> devices;
    for (const auto& pair : authorized_devices_) {
        if (pair.second.is_authorized) {
            devices.push_back(pair.second);
        }
    }
    return devices;
}

void MobileAPI::revokeDeviceAccess(const std::string& device_id) {
    auto it = authorized_devices_.find(device_id);
    if (it != authorized_devices_.end()) {
        it->second.is_authorized = false;
    }
    
    // Remove auth tokens for this device
    for (auto token_it = auth_tokens_.begin(); token_it != auth_tokens_.end();) {
        if (token_it->second == device_id) {
            token_it = auth_tokens_.erase(token_it);
        } else {
            ++token_it;
        }
    }
    
    // Disconnect WebSocket connections for this device
    if (ws_server_) {
        ws_server_->disconnectUser(device_id);
    }
}

void MobileAPI::updateDevicePermissions(const std::string& device_id, const std::string& permission_level) {
    auto it = authorized_devices_.find(device_id);
    if (it != authorized_devices_.end()) {
        it->second.permission_level = permission_level;
    }
}

PresentationState MobileAPI::getCurrentPresentationState() const {
    PresentationState state;
    
    // Get presentation state from the main app
    if (app_) {
        // These would need to be exposed in VerseFinderApp
        state.is_active = true; // app_->isPresentationModeActive();
        state.current_verse_text = "Sample verse text"; // app_->getCurrentDisplayedVerse();
        state.current_reference = "John 3:16"; // app_->getCurrentDisplayedReference();
        state.is_blank = false; // app_->isPresentationBlank();
    }
    
    return state;
}

bool MobileAPI::togglePresentationMode() {
    if (app_) {
        // app_->togglePresentationMode();
        notifyPresentationStateChange();
        return true;
    }
    return false;
}

bool MobileAPI::displayVerse(const std::string& verse_text, const std::string& reference) {
    if (app_) {
        // app_->displayVerseOnPresentation(verse_text, reference);
        notifyVerseChange(verse_text, reference);
        return true;
    }
    return false;
}

bool MobileAPI::toggleBlankScreen() {
    if (app_) {
        // app_->toggleBlankScreen();
        notifyPresentationStateChange();
        return true;
    }
    return false;
}

bool MobileAPI::showLogo() {
    if (app_) {
        // app_->showLogoOnPresentation();
        notifyPresentationStateChange();
        return true;
    }
    return false;
}

bool MobileAPI::navigateVerse(int direction) {
    if (app_) {
        // app_->navigateToVerse(direction);
        notifyPresentationStateChange();
        return true;
    }
    return false;
}

bool MobileAPI::setBackgroundTheme(const std::string& theme) {
    if (app_) {
        // app_->setPresentationTheme(theme);
        notifyPresentationStateChange();
        return true;
    }
    return false;
}

bool MobileAPI::adjustTextSize(float size_multiplier) {
    if (app_) {
        // app_->setPresentationTextSize(size_multiplier);
        notifyPresentationStateChange();
        return true;
    }
    return false;
}

bool MobileAPI::setTextPosition(const std::string& position) {
    if (app_) {
        // app_->setPresentationTextPosition(position);
        notifyPresentationStateChange();
        return true;
    }
    return false;
}

std::vector<std::string> MobileAPI::searchVerses(const std::string& query, const std::string& translation) const {
    if (bible_) {
        return bible_->searchByKeywords(query, translation);
    }
    return {};
}

std::string MobileAPI::getVerseText(const std::string& reference, const std::string& translation) const {
    if (bible_) {
        return bible_->searchByReference(reference, translation);
    }
    return "";
}

std::vector<std::string> MobileAPI::getAvailableTranslations() const {
    std::vector<std::string> translations;
    if (bible_) {
        const auto& translation_infos = bible_->getTranslations();
        for (const auto& info : translation_infos) {
            translations.push_back(info.abbreviation);
        }
    }
    return translations;
}

void MobileAPI::addToFavorites(const std::string& verse_reference, const std::string& user_id) {
    auto& favorites = user_favorites_[user_id];
    if (std::find(favorites.begin(), favorites.end(), verse_reference) == favorites.end()) {
        favorites.push_back(verse_reference);
    }
}

void MobileAPI::removeFromFavorites(const std::string& verse_reference, const std::string& user_id) {
    auto& favorites = user_favorites_[user_id];
    favorites.erase(std::remove(favorites.begin(), favorites.end(), verse_reference), favorites.end());
}

std::vector<std::string> MobileAPI::getUserFavorites(const std::string& user_id) const {
    auto it = user_favorites_.find(user_id);
    if (it != user_favorites_.end()) {
        return it->second;
    }
    return {};
}

void MobileAPI::notifyPresentationStateChange() {
    if (ws_server_) {
        PresentationState state = getCurrentPresentationState();
        std::string state_json = presentationStateToJson(state);
        ws_server_->broadcastMessage("presentation_state_changed", state_json);
    }
}

void MobileAPI::notifyVerseChange(const std::string& verse_text, const std::string& reference) {
    if (ws_server_) {
        std::string data = R"({"verse":")" + verse_text + R"(","reference":")" + reference + R"("})";
        ws_server_->broadcastMessage("verse_changed", data);
    }
}

void MobileAPI::notifyDeviceConnected(const std::string& device_name) {
    if (ws_server_) {
        std::string data = R"({"device":")" + device_name + R"("})";
        ws_server_->broadcastMessage("device_connected", data);
    }
}

void MobileAPI::notifyDeviceDisconnected(const std::string& device_name) {
    if (ws_server_) {
        std::string data = R"({"device":")" + device_name + R"("})";
        ws_server_->broadcastMessage("device_disconnected", data);
    }
}

void MobileAPI::updateMobileSettings(const std::string& settings_json) {
    mobile_settings_json_ = settings_json;
}

std::string MobileAPI::getMobileSettings() const {
    return mobile_settings_json_;
}

std::vector<std::string> MobileAPI::getEmergencyVerses() const {
    return emergency_verses_;
}

std::vector<std::string> MobileAPI::getQuickAccessVerses() const {
    return quick_access_verses_;
}

void MobileAPI::addQuickAccessVerse(const std::string& verse_reference) {
    if (std::find(quick_access_verses_.begin(), quick_access_verses_.end(), verse_reference) == quick_access_verses_.end()) {
        quick_access_verses_.push_back(verse_reference);
    }
}

void MobileAPI::removeQuickAccessVerse(const std::string& verse_reference) {
    quick_access_verses_.erase(
        std::remove(quick_access_verses_.begin(), quick_access_verses_.end(), verse_reference),
        quick_access_verses_.end()
    );
}

// API endpoint handlers
ApiResponse MobileAPI::handlePairingRequest(const ApiRequest& request) {
    std::string device_name = parseJsonString(request.body, "device_name");
    if (device_name.empty()) {
        return errorResponse(400, "Device name is required");
    }
    
    std::string session_id = createPairingSession(device_name);
    
    // Get the PIN for this session
    std::string pin;
    auto it = pairing_sessions_.find(session_id);
    if (it != pairing_sessions_.end()) {
        pin = it->second.pin_code;
    }
    
    std::string response_json = R"({"session_id":")" + session_id + 
                               R"(","pin":")" + pin + R"("})";
    
    return jsonResponse(response_json);
}

ApiResponse MobileAPI::handlePairingValidation(const ApiRequest& request) {
    std::string session_id = parseJsonString(request.body, "session_id");
    std::string pin = parseJsonString(request.body, "pin");
    
    if (validatePairingPin(session_id, pin)) {
        // Create device ID and register device
        std::string device_id = generateAuthToken(); // Reuse the token generator for device ID
        
        auto session_it = pairing_sessions_.find(session_id);
        if (session_it != pairing_sessions_.end()) {
            MobileDevice device;
            device.device_id = device_id;
            device.device_name = session_it->second.device_name;
            device.last_ip = request.client_ip;
            device.last_seen = std::chrono::steady_clock::now();
            device.is_authorized = true;
            device.permission_level = "user";
            
            registerDevice(device);
            
            std::string response_json = R"({"success":true,"device_id":")" + device_id + R"("})";
            return jsonResponse(response_json);
        }
    }
    
    return errorResponse(401, "Invalid session or PIN");
}

ApiResponse MobileAPI::handleAuthTokenRequest(const ApiRequest& request) {
    std::string device_id = parseJsonString(request.body, "device_id");
    std::string user_id = parseJsonString(request.body, "user_id");
    
    if (device_id.empty() || user_id.empty()) {
        return errorResponse(400, "Device ID and user ID are required");
    }
    
    // Check if device is authorized
    auto it = authorized_devices_.find(device_id);
    if (it == authorized_devices_.end() || !it->second.is_authorized) {
        return errorResponse(401, "Device not authorized");
    }
    
    std::string token = generateAuthToken(device_id, user_id);
    std::string response_json = R"({"token":")" + token + R"("})";
    
    return jsonResponse(response_json);
}

ApiResponse MobileAPI::handleDeviceStatus(const ApiRequest& request) {
    (void)request; // Suppress unused parameter warning
    
    std::vector<MobileDevice> devices = getAuthorizedDevices();
    std::string devices_json = "[";
    
    for (size_t i = 0; i < devices.size(); ++i) {
        if (i > 0) devices_json += ",";
        devices_json += deviceToJson(devices[i]);
    }
    
    devices_json += "]";
    
    return jsonResponse(devices_json);
}

ApiResponse MobileAPI::handlePresentationControl(const ApiRequest& request) {
    if (request.path.find("/status") != std::string::npos) {
        PresentationState state = getCurrentPresentationState();
        return jsonResponse(presentationStateToJson(state));
    }
    
    if (request.path.find("/toggle") != std::string::npos) {
        bool success = togglePresentationMode();
        std::string response = R"({"success":)" + std::string(success ? "true" : "false") + "}";
        return jsonResponse(response);
    }
    
    if (request.path.find("/blank") != std::string::npos) {
        bool success = toggleBlankScreen();
        std::string response = R"({"success":)" + std::string(success ? "true" : "false") + "}";
        return jsonResponse(response);
    }
    
    if (request.path.find("/navigate") != std::string::npos) {
        int direction = parseJsonInt(request.body, "direction");
        bool success = navigateVerse(direction);
        std::string response = R"({"success":)" + std::string(success ? "true" : "false") + "}";
        return jsonResponse(response);
    }
    
    return errorResponse(404, "Unknown presentation control endpoint");
}

ApiResponse MobileAPI::handleVerseSearch(const ApiRequest& request) {
    if (request.path.find("/translations") != std::string::npos) {
        std::vector<std::string> translations = getAvailableTranslations();
        std::string json = "[";
        for (size_t i = 0; i < translations.size(); ++i) {
            if (i > 0) json += ",";
            json += "\"" + translations[i] + "\"";
        }
        json += "]";
        return jsonResponse(json);
    }
    
    auto query_it = request.query_params.find("q");
    auto translation_it = request.query_params.find("translation");
    
    if (query_it == request.query_params.end()) {
        return errorResponse(400, "Query parameter 'q' is required");
    }
    
    std::string translation = (translation_it != request.query_params.end()) ? 
                             translation_it->second : "KJV";
    
    std::vector<std::string> results = searchVerses(query_it->second, translation);
    
    std::string json = "[";
    for (size_t i = 0; i < results.size() && i < 20; ++i) { // Limit to 20 results
        if (i > 0) json += ",";
        json += "\"" + results[i] + "\"";
    }
    json += "]";
    
    return jsonResponse(json);
}

ApiResponse MobileAPI::handleVerseDisplay(const ApiRequest& request) {
    std::string verse_text = parseJsonString(request.body, "verse_text");
    std::string reference = parseJsonString(request.body, "reference");
    
    bool success = displayVerse(verse_text, reference);
    std::string response = R"({"success":)" + std::string(success ? "true" : "false") + "}";
    
    return jsonResponse(response);
}

ApiResponse MobileAPI::handleFavorites(const ApiRequest& request) {
    if (request.method == HttpMethod::GET) {
        std::vector<std::string> favorites = getUserFavorites(request.user_id);
        std::string json = "[";
        for (size_t i = 0; i < favorites.size(); ++i) {
            if (i > 0) json += ",";
            json += "\"" + favorites[i] + "\"";
        }
        json += "]";
        return jsonResponse(json);
    }
    
    std::string verse_reference = parseJsonString(request.body, "verse_reference");
    if (verse_reference.empty()) {
        return errorResponse(400, "Verse reference is required");
    }
    
    if (request.method == HttpMethod::POST) {
        addToFavorites(verse_reference, request.user_id);
    } else if (request.method == HttpMethod::DELETE) {
        removeFromFavorites(verse_reference, request.user_id);
    }
    
    return successResponse("Favorites updated");
}

ApiResponse MobileAPI::handleQuickAccess(const ApiRequest& request) {
    if (request.method == HttpMethod::GET) {
        std::vector<std::string> verses = getQuickAccessVerses();
        std::string json = "[";
        for (size_t i = 0; i < verses.size(); ++i) {
            if (i > 0) json += ",";
            json += "\"" + verses[i] + "\"";
        }
        json += "]";
        return jsonResponse(json);
    }
    
    std::string verse_reference = parseJsonString(request.body, "verse_reference");
    if (verse_reference.empty()) {
        return errorResponse(400, "Verse reference is required");
    }
    
    addQuickAccessVerse(verse_reference);
    return successResponse("Quick access updated");
}

ApiResponse MobileAPI::handleEmergency(const ApiRequest& request) {
    (void)request; // Suppress unused parameter warning
    
    std::vector<std::string> verses = getEmergencyVerses();
    std::string json = "[";
    for (size_t i = 0; i < verses.size(); ++i) {
        if (i > 0) json += ",";
        json += "\"" + verses[i] + "\"";
    }
    json += "]";
    
    return jsonResponse(json);
}

ApiResponse MobileAPI::handleSettings(const ApiRequest& request) {
    if (request.method == HttpMethod::GET) {
        return jsonResponse(getMobileSettings());
    }
    
    updateMobileSettings(request.body);
    return successResponse("Settings updated");
}

// WebSocket message handlers
void MobileAPI::handleWebSocketAuth(const std::string& connection_id, const WebSocketMessage& message) {
    std::string token = parseJsonString(message.data, "token");
    if (ws_server_ && ws_server_->authenticateConnection(connection_id, token)) {
        ws_server_->sendToConnection(connection_id, "auth_success", R"({"authenticated":true})");
    } else {
        ws_server_->sendToConnection(connection_id, "auth_error", R"({"error":"Invalid token"})");
    }
}

void MobileAPI::handlePresentationCommand(const std::string& connection_id, const WebSocketMessage& message) {
    (void)connection_id; // Suppress unused parameter warning
    
    std::string command = parseJsonString(message.data, "command");
    
    if (command == "toggle") {
        togglePresentationMode();
    } else if (command == "blank") {
        toggleBlankScreen();
    } else if (command == "navigate") {
        int direction = parseJsonInt(message.data, "direction");
        navigateVerse(direction);
    } else if (command == "theme") {
        std::string theme = parseJsonString(message.data, "theme");
        setBackgroundTheme(theme);
    }
}

void MobileAPI::handleSearchRequest(const std::string& connection_id, const WebSocketMessage& message) {
    std::string query = parseJsonString(message.data, "query");
    std::string translation = parseJsonString(message.data, "translation");
    
    if (translation.empty()) translation = "KJV";
    
    std::vector<std::string> results = searchVerses(query, translation);
    
    std::string results_json = "[";
    for (size_t i = 0; i < results.size() && i < 20; ++i) {
        if (i > 0) results_json += ",";
        results_json += "\"" + results[i] + "\"";
    }
    results_json += "]";
    
    if (ws_server_) {
        ws_server_->sendToConnection(connection_id, "search_results", results_json);
    }
}

void MobileAPI::handleSubscribeEvents(const std::string& connection_id, const WebSocketMessage& message) {
    std::string event = parseJsonString(message.data, "event");
    if (ws_server_) {
        ws_server_->subscribeToEvent(connection_id, event);
    }
}

void MobileAPI::handleHeartbeat(const std::string& connection_id, const WebSocketMessage& message) {
    (void)message; // Suppress unused parameter warning
    
    if (ws_server_) {
        ws_server_->sendToConnection(connection_id, "heartbeat_response", R"({"timestamp":)" + 
            std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()) + "}");
    }
}

// Utility methods
std::string MobileAPI::generatePinCode() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);
    
    return std::to_string(dis(gen));
}

std::string MobileAPI::generateSessionId() {
    return generateAuthToken(); // Reuse the auth token generator
}

std::string MobileAPI::generateAuthToken() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    for (int i = 0; i < 64; ++i) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

bool MobileAPI::hasPermission(const std::string& device_id, const std::string& required_permission) const {
    auto it = authorized_devices_.find(device_id);
    if (it != authorized_devices_.end()) {
        const std::string& level = it->second.permission_level;
        
        if (required_permission == "user") return true;
        if (required_permission == "presenter") return level == "presenter" || level == "admin";
        if (required_permission == "admin") return level == "admin";
    }
    return false;
}

void MobileAPI::cleanupExpiredSessions() {
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = pairing_sessions_.begin(); it != pairing_sessions_.end();) {
        auto age = std::chrono::duration_cast<std::chrono::minutes>(now - it->second.created_at).count();
        if (age > 10) { // 10 minute expiry
            it = pairing_sessions_.erase(it);
        } else {
            ++it;
        }
    }
}

std::string MobileAPI::deviceToJson(const MobileDevice& device) const {
    return R"({"device_id":")" + device.device_id + 
           R"(","device_name":")" + device.device_name +
           R"(","user_id":")" + device.user_id +
           R"(","permission_level":")" + device.permission_level +
           R"(","is_authorized":)" + (device.is_authorized ? "true" : "false") + "}";
}

std::string MobileAPI::presentationStateToJson(const PresentationState& state) const {
    return R"({"is_active":)" + std::string(state.is_active ? "true" : "false") +
           R"(,"current_verse":")" + state.current_verse_text +
           R"(","current_reference":")" + state.current_reference +
           R"(","is_blank":)" + std::string(state.is_blank ? "true" : "false") +
           R"(,"is_logo_displayed":)" + std::string(state.is_logo_displayed ? "true" : "false") +
           R"(,"background_theme":")" + state.background_theme +
           R"(","text_size":)" + std::to_string(state.text_size) +
           R"(,"text_position":")" + state.text_position + R"("})";
}

std::string MobileAPI::parseJsonString(const std::string& json, const std::string& key) const {
    std::string search_key = "\"" + key + "\":\"";
    size_t start = json.find(search_key);
    if (start != std::string::npos) {
        start += search_key.length();
        size_t end = json.find("\"", start);
        if (end != std::string::npos) {
            return json.substr(start, end - start);
        }
    }
    return "";
}

int MobileAPI::parseJsonInt(const std::string& json, const std::string& key) const {
    std::string search_key = "\"" + key + "\":";
    size_t start = json.find(search_key);
    if (start != std::string::npos) {
        start += search_key.length();
        size_t end = json.find_first_of(",}", start);
        if (end != std::string::npos) {
            std::string value = json.substr(start, end - start);
            try {
                return std::stoi(value);
            } catch (...) {
                // Ignore parsing errors
            }
        }
    }
    return 0;
}

bool MobileAPI::parseJsonBool(const std::string& json, const std::string& key) const {
    std::string search_key = "\"" + key + "\":";
    size_t start = json.find(search_key);
    if (start != std::string::npos) {
        start += search_key.length();
        return json.substr(start, 4) == "true";
    }
    return false;
}