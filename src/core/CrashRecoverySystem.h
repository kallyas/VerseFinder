#ifndef CRASH_RECOVERY_SYSTEM_H
#define CRASH_RECOVERY_SYSTEM_H

#include <string>
#include <vector>
#include <chrono>
#include <mutex>
#include <atomic>
#include <memory>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

struct SessionState {
    std::string current_translation;
    std::string current_search_query;
    std::vector<std::string> search_history;
    std::vector<std::string> selected_verses;
    std::vector<std::string> favorite_verses;
    std::map<std::string, std::vector<std::string>> custom_collections;
    json presentation_settings;
    json ui_settings;
    bool presentation_mode_active = false;
    std::string current_displayed_verse;
    std::chrono::system_clock::time_point timestamp;
    std::string session_id;
};

struct RecoveryStats {
    int successful_recoveries = 0;
    int failed_recoveries = 0;
    int sessions_created = 0;
    int sessions_recovered = 0;
    std::chrono::seconds average_recovery_time{0};
};

class CrashRecoverySystem {
private:
    std::string recovery_directory;
    std::string current_session_file;
    std::string backup_session_file;
    std::atomic<bool> is_initialized{false};
    std::mutex state_mutex;
    
    // Session management
    SessionState current_session;
    std::string session_id;
    
    // Recovery statistics
    RecoveryStats stats;
    std::mutex stats_mutex;
    
    // Configuration
    int max_session_backups = 5;
    std::chrono::hours session_retention_period{24 * 7}; // 7 days
    
    // Internal methods
    std::string generateSessionId();
    std::string getSessionFilePath(const std::string& session_id);
    bool saveSessionToFile(const SessionState& state, const std::string& filepath);
    bool loadSessionFromFile(const std::string& filepath, SessionState& state);
    void cleanupOldSessions();
    bool validateSessionData(const SessionState& state);
    
public:
    CrashRecoverySystem();
    ~CrashRecoverySystem();
    
    // Initialization and cleanup
    bool initialize(const std::string& recovery_dir);
    void shutdown();
    
    // Session state management
    bool saveSessionState();
    bool loadLastSession(std::string& session_data);
    bool hasRecoverableSession();
    
    // State updates
    void updateCurrentTranslation(const std::string& translation);
    void updateSearchQuery(const std::string& query);
    void addToSearchHistory(const std::string& query);
    void updateSelectedVerses(const std::vector<std::string>& verses);
    void updateFavoriteVerses(const std::vector<std::string>& verses);
    void updateCustomCollections(const std::map<std::string, std::vector<std::string>>& collections);
    void updatePresentationSettings(const json& settings);
    void updateUISettings(const json& settings);
    void setPresentationMode(bool active, const std::string& displayed_verse = "");
    
    // Recovery operations
    bool recoverSession(const std::string& session_id = "");
    std::vector<std::string> getAvailableSessions();
    bool deleteSession(const std::string& session_id);
    
    // Session information
    SessionState getCurrentSessionState();
    std::string getCurrentSessionId() const;
    std::chrono::system_clock::time_point getLastSaveTime();
    
    // Configuration
    void setMaxSessionBackups(int max_backups);
    void setSessionRetentionPeriod(std::chrono::hours hours);
    
    // Maintenance and diagnostics
    bool selfTest();
    void resetStats();
    RecoveryStats getStats();
    std::string generateReport();
    bool cleanupOldFiles();
    
    // Emergency recovery
    bool createEmergencySnapshot();
    bool restoreFromEmergencySnapshot();
    
    // Data validation and repair
    bool validateAllSessions();
    bool repairCorruptedSession(const std::string& session_id);
    
    // Export and import
    bool exportSession(const std::string& session_id, const std::string& export_path);
    bool importSession(const std::string& import_path);
    
    // Version compatibility
    bool upgradeSessionFormat(const std::string& session_id);
    bool isSessionCompatible(const std::string& session_id);
};

#endif // CRASH_RECOVERY_SYSTEM_H