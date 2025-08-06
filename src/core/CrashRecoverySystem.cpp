#include "CrashRecoverySystem.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>

CrashRecoverySystem::CrashRecoverySystem() {
    session_id = generateSessionId();
    current_session.session_id = session_id;
    current_session.timestamp = std::chrono::system_clock::now();
}

CrashRecoverySystem::~CrashRecoverySystem() {
    shutdown();
}

bool CrashRecoverySystem::initialize(const std::string& recovery_dir) {
    if (is_initialized.load()) {
        return true;
    }
    
    try {
        recovery_directory = recovery_dir;
        
        // Create recovery directory if it doesn't exist
        std::filesystem::create_directories(recovery_directory);
        
        // Set up file paths
        current_session_file = recovery_directory + "/current_session.json";
        backup_session_file = recovery_directory + "/backup_session.json";
        
        // Clean up old sessions
        cleanupOldSessions();
        
        // Initialize current session
        current_session.timestamp = std::chrono::system_clock::now();
        
        is_initialized.store(true);
        
        std::cout << "CrashRecoverySystem initialized with session ID: " << session_id << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize CrashRecoverySystem: " << e.what() << std::endl;
        return false;
    }
}

void CrashRecoverySystem::shutdown() {
    if (!is_initialized.load()) {
        return;
    }
    
    // Save final state
    saveSessionState();
    
    is_initialized.store(false);
    std::cout << "CrashRecoverySystem shutdown complete" << std::endl;
}

std::string CrashRecoverySystem::generateSessionId() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    ss << "_" << dis(gen);
    
    return ss.str();
}

std::string CrashRecoverySystem::getSessionFilePath(const std::string& session_id) {
    return recovery_directory + "/session_" + session_id + ".json";
}

bool CrashRecoverySystem::saveSessionToFile(const SessionState& state, const std::string& filepath) {
    try {
        json session_json;
        
        // Convert SessionState to JSON
        session_json["session_id"] = state.session_id;
        session_json["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
            state.timestamp.time_since_epoch()).count();
        session_json["current_translation"] = state.current_translation;
        session_json["current_search_query"] = state.current_search_query;
        session_json["search_history"] = state.search_history;
        session_json["selected_verses"] = state.selected_verses;
        session_json["favorite_verses"] = state.favorite_verses;
        session_json["custom_collections"] = state.custom_collections;
        session_json["presentation_settings"] = state.presentation_settings;
        session_json["ui_settings"] = state.ui_settings;
        session_json["presentation_mode_active"] = state.presentation_mode_active;
        session_json["current_displayed_verse"] = state.current_displayed_verse;
        session_json["version"] = "1.0";
        
        // Write to file
        std::ofstream file(filepath);
        if (!file.is_open()) {
            return false;
        }
        
        file << session_json.dump(4);
        file.close();
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to save session to file: " << e.what() << std::endl;
        return false;
    }
}

bool CrashRecoverySystem::loadSessionFromFile(const std::string& filepath, SessionState& state) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }
        
        json session_json;
        file >> session_json;
        file.close();
        
        // Convert JSON to SessionState
        state.session_id = session_json.value("session_id", "");
        
        auto timestamp_seconds = session_json.value("timestamp", 0L);
        state.timestamp = std::chrono::system_clock::from_time_t(timestamp_seconds);
        
        state.current_translation = session_json.value("current_translation", "");
        state.current_search_query = session_json.value("current_search_query", "");
        state.search_history = session_json.value("search_history", std::vector<std::string>());
        state.selected_verses = session_json.value("selected_verses", std::vector<std::string>());
        state.favorite_verses = session_json.value("favorite_verses", std::vector<std::string>());
        
        if (session_json.contains("custom_collections")) {
            state.custom_collections = session_json["custom_collections"];
        }
        
        state.presentation_settings = session_json.value("presentation_settings", json::object());
        state.ui_settings = session_json.value("ui_settings", json::object());
        state.presentation_mode_active = session_json.value("presentation_mode_active", false);
        state.current_displayed_verse = session_json.value("current_displayed_verse", "");
        
        return validateSessionData(state);
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to load session from file: " << e.what() << std::endl;
        return false;
    }
}

bool CrashRecoverySystem::validateSessionData(const SessionState& state) {
    // Basic validation
    if (state.session_id.empty()) {
        return false;
    }
    
    // Check timestamp is reasonable (not too old or in the future)
    auto now = std::chrono::system_clock::now();
    auto age = now - state.timestamp;
    
    if (age > session_retention_period || age < std::chrono::seconds(0)) {
        return false;
    }
    
    // Validate JSON structures
    try {
        if (!state.presentation_settings.is_object() || !state.ui_settings.is_object()) {
            return false;
        }
    } catch (const std::exception&) {
        return false;
    }
    
    return true;
}

void CrashRecoverySystem::cleanupOldSessions() {
    try {
        auto now = std::chrono::system_clock::now();
        
        for (const auto& entry : std::filesystem::directory_iterator(recovery_directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                auto filename = entry.path().filename().string();
                
                // Skip current and backup files
                if (filename == "current_session.json" || filename == "backup_session.json") {
                    continue;
                }
                
                // Check file age
                auto file_time = std::filesystem::last_write_time(entry);
                auto file_age = now - std::chrono::system_clock::time_point(
                    std::chrono::duration_cast<std::chrono::system_clock::duration>(
                        file_time.time_since_epoch()));
                
                if (file_age > session_retention_period) {
                    std::filesystem::remove(entry.path());
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error during session cleanup: " << e.what() << std::endl;
    }
}

bool CrashRecoverySystem::saveSessionState() {
    if (!is_initialized.load()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(state_mutex);
    
    try {
        // Update timestamp
        current_session.timestamp = std::chrono::system_clock::now();
        
        // Save to current session file
        if (!saveSessionToFile(current_session, current_session_file)) {
            return false;
        }
        
        // Also save a backup copy
        saveSessionToFile(current_session, backup_session_file);
        
        // Save a timestamped copy
        std::string timestamped_file = getSessionFilePath(session_id);
        saveSessionToFile(current_session, timestamped_file);
        
        {
            std::lock_guard<std::mutex> stats_lock(stats_mutex);
            stats.sessions_created++;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to save session state: " << e.what() << std::endl;
        return false;
    }
}

bool CrashRecoverySystem::loadLastSession(std::string& session_data) {
    if (!is_initialized.load()) {
        return false;
    }
    
    SessionState loaded_state;
    bool success = false;
    
    // Try to load from current session file first
    if (std::filesystem::exists(current_session_file)) {
        success = loadSessionFromFile(current_session_file, loaded_state);
    }
    
    // If that fails, try the backup
    if (!success && std::filesystem::exists(backup_session_file)) {
        success = loadSessionFromFile(backup_session_file, loaded_state);
    }
    
    if (success) {
        std::lock_guard<std::mutex> lock(state_mutex);
        current_session = loaded_state;
        
        // Convert to JSON string for application
        json session_json;
        session_json["current_translation"] = loaded_state.current_translation;
        session_json["current_search_query"] = loaded_state.current_search_query;
        session_json["search_history"] = loaded_state.search_history;
        session_json["selected_verses"] = loaded_state.selected_verses;
        session_json["favorite_verses"] = loaded_state.favorite_verses;
        session_json["custom_collections"] = loaded_state.custom_collections;
        session_json["presentation_settings"] = loaded_state.presentation_settings;
        session_json["ui_settings"] = loaded_state.ui_settings;
        session_json["presentation_mode_active"] = loaded_state.presentation_mode_active;
        session_json["current_displayed_verse"] = loaded_state.current_displayed_verse;
        
        session_data = session_json.dump();
        
        {
            std::lock_guard<std::mutex> stats_lock(stats_mutex);
            stats.sessions_recovered++;
            stats.successful_recoveries++;
        }
        
        return true;
    }
    
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex);
        stats.failed_recoveries++;
    }
    
    return false;
}

bool CrashRecoverySystem::hasRecoverableSession() {
    if (!is_initialized.load()) {
        return false;
    }
    
    // Check if current session file exists and is valid
    if (std::filesystem::exists(current_session_file)) {
        SessionState test_state;
        return loadSessionFromFile(current_session_file, test_state);
    }
    
    // Check backup file
    if (std::filesystem::exists(backup_session_file)) {
        SessionState test_state;
        return loadSessionFromFile(backup_session_file, test_state);
    }
    
    return false;
}

// State update methods
void CrashRecoverySystem::updateCurrentTranslation(const std::string& translation) {
    std::lock_guard<std::mutex> lock(state_mutex);
    current_session.current_translation = translation;
}

void CrashRecoverySystem::updateSearchQuery(const std::string& query) {
    std::lock_guard<std::mutex> lock(state_mutex);
    current_session.current_search_query = query;
}

void CrashRecoverySystem::addToSearchHistory(const std::string& query) {
    std::lock_guard<std::mutex> lock(state_mutex);
    
    // Remove if already exists to avoid duplicates
    auto it = std::find(current_session.search_history.begin(), 
                       current_session.search_history.end(), query);
    if (it != current_session.search_history.end()) {
        current_session.search_history.erase(it);
    }
    
    // Add to front
    current_session.search_history.insert(current_session.search_history.begin(), query);
    
    // Limit history size
    if (current_session.search_history.size() > 50) {
        current_session.search_history.resize(50);
    }
}

void CrashRecoverySystem::updateSelectedVerses(const std::vector<std::string>& verses) {
    std::lock_guard<std::mutex> lock(state_mutex);
    current_session.selected_verses = verses;
}

void CrashRecoverySystem::updateFavoriteVerses(const std::vector<std::string>& verses) {
    std::lock_guard<std::mutex> lock(state_mutex);
    current_session.favorite_verses = verses;
}

void CrashRecoverySystem::updateCustomCollections(const std::map<std::string, std::vector<std::string>>& collections) {
    std::lock_guard<std::mutex> lock(state_mutex);
    current_session.custom_collections = collections;
}

void CrashRecoverySystem::updatePresentationSettings(const json& settings) {
    std::lock_guard<std::mutex> lock(state_mutex);
    current_session.presentation_settings = settings;
}

void CrashRecoverySystem::updateUISettings(const json& settings) {
    std::lock_guard<std::mutex> lock(state_mutex);
    current_session.ui_settings = settings;
}

void CrashRecoverySystem::setPresentationMode(bool active, const std::string& displayed_verse) {
    std::lock_guard<std::mutex> lock(state_mutex);
    current_session.presentation_mode_active = active;
    current_session.current_displayed_verse = displayed_verse;
}

bool CrashRecoverySystem::recoverSession(const std::string& session_id) {
    if (!is_initialized.load()) {
        return false;
    }
    
    std::string filepath;
    if (session_id.empty()) {
        // Use current session file
        filepath = current_session_file;
    } else {
        filepath = getSessionFilePath(session_id);
    }
    
    SessionState recovered_state;
    if (loadSessionFromFile(filepath, recovered_state)) {
        std::lock_guard<std::mutex> lock(state_mutex);
        current_session = recovered_state;
        return true;
    }
    
    return false;
}

std::vector<std::string> CrashRecoverySystem::getAvailableSessions() {
    std::vector<std::string> sessions;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(recovery_directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                auto filename = entry.path().filename().string();
                
                // Extract session ID from filename
                if (filename.starts_with("session_") && filename.ends_with(".json")) {
                    std::string session_id = filename.substr(8); // Remove "session_"
                    session_id = session_id.substr(0, session_id.length() - 5); // Remove ".json"
                    sessions.push_back(session_id);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error getting available sessions: " << e.what() << std::endl;
    }
    
    return sessions;
}

bool CrashRecoverySystem::deleteSession(const std::string& session_id) {
    try {
        std::string filepath = getSessionFilePath(session_id);
        return std::filesystem::remove(filepath);
    } catch (const std::exception& e) {
        std::cerr << "Error deleting session: " << e.what() << std::endl;
        return false;
    }
}

SessionState CrashRecoverySystem::getCurrentSessionState() {
    std::lock_guard<std::mutex> lock(state_mutex);
    return current_session;
}

std::string CrashRecoverySystem::getCurrentSessionId() const {
    return session_id;
}

std::chrono::system_clock::time_point CrashRecoverySystem::getLastSaveTime() {
    std::lock_guard<std::mutex> lock(state_mutex);
    return current_session.timestamp;
}

void CrashRecoverySystem::setMaxSessionBackups(int max_backups) {
    max_session_backups = max_backups;
}

void CrashRecoverySystem::setSessionRetentionPeriod(std::chrono::hours hours) {
    session_retention_period = hours;
}

bool CrashRecoverySystem::selfTest() {
    if (!is_initialized.load()) {
        return false;
    }
    
    try {
        // Test save and load
        SessionState test_state = current_session;
        test_state.session_id = "test_" + generateSessionId();
        
        std::string test_file = recovery_directory + "/test_session.json";
        
        // Save test state
        if (!saveSessionToFile(test_state, test_file)) {
            return false;
        }
        
        // Load test state
        SessionState loaded_state;
        if (!loadSessionFromFile(test_file, loaded_state)) {
            std::filesystem::remove(test_file);
            return false;
        }
        
        // Verify data integrity
        bool integrity_ok = (loaded_state.session_id == test_state.session_id);
        
        // Cleanup
        std::filesystem::remove(test_file);
        
        return integrity_ok;
        
    } catch (const std::exception& e) {
        std::cerr << "CrashRecoverySystem self-test failed: " << e.what() << std::endl;
        return false;
    }
}

void CrashRecoverySystem::resetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex);
    stats = RecoveryStats();
}

RecoveryStats CrashRecoverySystem::getStats() {
    std::lock_guard<std::mutex> lock(stats_mutex);
    return stats;
}

std::string CrashRecoverySystem::generateReport() {
    std::string report = "\n=== Crash Recovery System Report ===\n";
    
    auto stats = getStats();
    report += "Sessions Created: " + std::to_string(stats.sessions_created) + "\n";
    report += "Sessions Recovered: " + std::to_string(stats.sessions_recovered) + "\n";
    report += "Successful Recoveries: " + std::to_string(stats.successful_recoveries) + "\n";
    report += "Failed Recoveries: " + std::to_string(stats.failed_recoveries) + "\n";
    
    auto available_sessions = getAvailableSessions();
    report += "Available Sessions: " + std::to_string(available_sessions.size()) + "\n";
    
    report += "Current Session ID: " + session_id + "\n";
    
    auto last_save = getLastSaveTime();
    auto now = std::chrono::system_clock::now();
    auto time_since_save = std::chrono::duration_cast<std::chrono::seconds>(now - last_save);
    report += "Time Since Last Save: " + std::to_string(time_since_save.count()) + " seconds\n";
    
    return report;
}

bool CrashRecoverySystem::cleanupOldFiles() {
    try {
        cleanupOldSessions();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error cleaning up old files: " << e.what() << std::endl;
        return false;
    }
}

bool CrashRecoverySystem::createEmergencySnapshot() {
    try {
        std::string emergency_file = recovery_directory + "/emergency_snapshot.json";
        std::lock_guard<std::mutex> lock(state_mutex);
        return saveSessionToFile(current_session, emergency_file);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create emergency snapshot: " << e.what() << std::endl;
        return false;
    }
}

bool CrashRecoverySystem::restoreFromEmergencySnapshot() {
    try {
        std::string emergency_file = recovery_directory + "/emergency_snapshot.json";
        SessionState emergency_state;
        
        if (loadSessionFromFile(emergency_file, emergency_state)) {
            std::lock_guard<std::mutex> lock(state_mutex);
            current_session = emergency_state;
            return true;
        }
        
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Failed to restore from emergency snapshot: " << e.what() << std::endl;
        return false;
    }
}

bool CrashRecoverySystem::validateAllSessions() {
    bool all_valid = true;
    
    for (const auto& session_id : getAvailableSessions()) {
        SessionState state;
        std::string filepath = getSessionFilePath(session_id);
        
        if (!loadSessionFromFile(filepath, state)) {
            all_valid = false;
            std::cerr << "Invalid session: " << session_id << std::endl;
        }
    }
    
    return all_valid;
}

bool CrashRecoverySystem::repairCorruptedSession(const std::string& session_id) {
    // This is a simplified repair - in practice, you might implement
    // more sophisticated recovery techniques
    try {
        std::string filepath = getSessionFilePath(session_id);
        
        // Try to read the file as text and fix common JSON issues
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        // Basic repairs (this could be expanded)
        // Fix missing closing braces, quotes, etc.
        
        // Try to parse as JSON
        try {
            json test_json = json::parse(content);
            return true; // Already valid
        } catch (const std::exception&) {
            // Could implement more sophisticated repair logic here
            return false;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error repairing session: " << e.what() << std::endl;
        return false;
    }
}

bool CrashRecoverySystem::exportSession(const std::string& session_id, const std::string& export_path) {
    try {
        std::string source_path = getSessionFilePath(session_id);
        std::filesystem::copy_file(source_path, export_path, 
                                  std::filesystem::copy_options::overwrite_existing);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error exporting session: " << e.what() << std::endl;
        return false;
    }
}

bool CrashRecoverySystem::importSession(const std::string& import_path) {
    try {
        // Load and validate the imported session
        SessionState imported_state;
        if (!loadSessionFromFile(import_path, imported_state)) {
            return false;
        }
        
        // Generate new session ID and save
        imported_state.session_id = generateSessionId();
        imported_state.timestamp = std::chrono::system_clock::now();
        
        std::string new_filepath = getSessionFilePath(imported_state.session_id);
        return saveSessionToFile(imported_state, new_filepath);
        
    } catch (const std::exception& e) {
        std::cerr << "Error importing session: " << e.what() << std::endl;
        return false;
    }
}

bool CrashRecoverySystem::upgradeSessionFormat([[maybe_unused]] const std::string& session_id) {
    // This would handle format upgrades between versions
    // For now, just return true since we're using version 1.0
    return true;
}

bool CrashRecoverySystem::isSessionCompatible(const std::string& session_id) {
    try {
        std::string filepath = getSessionFilePath(session_id);
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }
        
        json session_json;
        file >> session_json;
        file.close();
        
        // Check version compatibility
        std::string version = session_json.value("version", "1.0");
        return version == "1.0"; // Only support current version for now
        
    } catch (const std::exception& e) {
        std::cerr << "Error checking session compatibility: " << e.what() << std::endl;
        return false;
    }
}