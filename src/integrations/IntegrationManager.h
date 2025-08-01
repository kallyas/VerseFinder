#ifndef INTEGRATION_MANAGER_H
#define INTEGRATION_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <mutex>

// Forward declarations
class ServicePlan;
class IntegrationProvider;

enum class IntegrationStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR,
    SYNCING
};

enum class IntegrationType {
    PLANNING_CENTER,
    CHURCH_TOOLS,
    BREEZE_CHMS,
    ROCK_RMS,
    ELVANTO,
    CHURCH_COMMUNITY_BUILDER,
    PROPRESENTER,
    EASY_WORSHIP,
    MEDIA_SHOUT,
    OPEN_LP,
    PROCLAIM
};

struct IntegrationConfig {
    IntegrationType type;
    std::string name;
    std::string endpoint;
    std::string api_key;
    std::string client_id;
    std::string client_secret;
    bool auto_sync;
    int sync_interval_minutes;
};

struct IntegrationInfo {
    IntegrationType type;
    std::string name;
    std::string description;
    std::string icon_path;
    bool requires_oauth;
    bool supports_export;
    bool supports_import;
    bool supports_realtime;
};

class IntegrationManager {
public:
    IntegrationManager();
    ~IntegrationManager();

    // Core integration management
    bool addIntegration(const IntegrationConfig& config);
    bool removeIntegration(IntegrationType type);
    bool testConnection(IntegrationType type);
    IntegrationStatus getStatus(IntegrationType type) const;
    
    // Service plan synchronization
    bool exportServicePlan(const ServicePlan& plan, IntegrationType target);
    bool importServicePlan(IntegrationType source, ServicePlan& plan);
    bool syncServicePlans(IntegrationType type);
    
    // Real-time features
    void enableRealTimeSync(IntegrationType type, bool enable);
    void setStatusCallback(std::function<void(IntegrationType, IntegrationStatus)> callback);
    
    // Configuration
    std::vector<IntegrationInfo> getAvailableIntegrations() const;
    std::vector<IntegrationType> getActiveIntegrations() const;
    IntegrationConfig getConfig(IntegrationType type) const;
    bool updateConfig(const IntegrationConfig& config);
    
    // OAuth handling
    std::string generateOAuthUrl(IntegrationType type) const;
    bool handleOAuthCallback(IntegrationType type, const std::string& code);
    
    // Error handling
    std::string getLastError(IntegrationType type) const;
    void clearErrors(IntegrationType type);

private:
    std::unordered_map<IntegrationType, std::unique_ptr<IntegrationProvider>> providers_;
    std::unordered_map<IntegrationType, IntegrationConfig> configs_;
    std::unordered_map<IntegrationType, IntegrationStatus> statuses_;
    std::unordered_map<IntegrationType, std::string> errors_;
    std::function<void(IntegrationType, IntegrationStatus)> status_callback_;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    void initializeProviders();
    void updateStatus(IntegrationType type, IntegrationStatus status);
    std::string typeToString(IntegrationType type) const;
};

#endif // INTEGRATION_MANAGER_H