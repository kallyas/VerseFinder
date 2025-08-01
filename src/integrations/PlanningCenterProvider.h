#ifndef PLANNING_CENTER_PROVIDER_H
#define PLANNING_CENTER_PROVIDER_H

#include "IntegrationProvider.h"
#include <string>
#include <chrono>

class PlanningCenterProvider : public IntegrationProvider {
public:
    PlanningCenterProvider();
    ~PlanningCenterProvider() override = default;
    
    bool testConnection(const IntegrationConfig& config) override;
    bool exportServicePlan(const ServicePlan& plan, const IntegrationConfig& config) override;
    bool importServicePlan(ServicePlan& plan, const IntegrationConfig& config) override;
    std::string generateOAuthUrl() const override;
    bool handleOAuthCallback(const std::string& code, IntegrationConfig& config) override;
    std::string getLastError() const override;
    
    // Planning Center specific methods
    bool syncServiceOrders(const IntegrationConfig& config);
    bool importScriptureReadings(const IntegrationConfig& config, ServicePlan& plan);
    bool exportVerseCollections(const ServicePlan& plan, const IntegrationConfig& config);
    std::vector<std::string> getAvailableServices(const IntegrationConfig& config);
    
private:
    struct PCOService {
        std::string id;
        std::string name;
        std::chrono::system_clock::time_point service_time;
        std::string series_title;
        std::string plan_title;
    };
    
    struct PCOItem {
        std::string id;
        std::string title;
        std::string category;
        std::string description;
        std::chrono::seconds length;
        std::string assigned_to;
        std::vector<std::string> arrangements;
        std::vector<std::string> attachments;
    };
    
    bool makeApiRequest(const std::string& endpoint, const std::string& method,
                       const std::string& body, std::string& response,
                       const IntegrationConfig& config);
    
    std::string buildAuthHeader(const IntegrationConfig& config) const;
    bool refreshAccessToken(IntegrationConfig& config);
    
    std::vector<PCOService> parseServicesResponse(const std::string& response);
    std::vector<PCOItem> parseItemsResponse(const std::string& response);
    ServicePlan convertPCOToServicePlan(const PCOService& service, const std::vector<PCOItem>& items);
    std::string convertServicePlanToPCO(const ServicePlan& plan);
    
    // OAuth configuration
    static const std::string CLIENT_ID;
    static const std::string REDIRECT_URI;
    static const std::string API_BASE_URL;
    static const std::string OAUTH_BASE_URL;
};

#endif // PLANNING_CENTER_PROVIDER_H