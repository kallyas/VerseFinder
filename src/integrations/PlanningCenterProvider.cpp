#include "PlanningCenterProvider.h"
#include "../service/ServicePlan.h"
#include <sstream>
#include <regex>

// Planning Center OAuth configuration
const std::string PlanningCenterProvider::CLIENT_ID = "your_pco_client_id";
const std::string PlanningCenterProvider::REDIRECT_URI = "http://localhost:8080/auth/pco/callback";
const std::string PlanningCenterProvider::API_BASE_URL = "https://api.planningcenteronline.com/services/v2";
const std::string PlanningCenterProvider::OAUTH_BASE_URL = "https://api.planningcenteronline.com/oauth";

PlanningCenterProvider::PlanningCenterProvider() {
    // Initialize provider
}

bool PlanningCenterProvider::testConnection(const IntegrationConfig& config) {
    std::string response;
    bool success = makeApiRequest("/service_types", "GET", "", response, config);
    
    if (!success) {
        last_error_ = "Failed to connect to Planning Center API";
        return false;
    }
    
    // Check if response indicates valid authentication
    if (response.find("\"data\"") != std::string::npos) {
        return true;
    }
    
    last_error_ = "Authentication failed with Planning Center";
    return false;
}

bool PlanningCenterProvider::exportServicePlan(const ServicePlan& plan, const IntegrationConfig& config) {
    try {
        std::string pco_json = convertServicePlanToPCO(plan);
        std::string response;
        
        // First, create the plan
        bool success = makeApiRequest("/service_types/1/plans", "POST", pco_json, response, config);
        
        if (!success) {
            last_error_ = "Failed to create service plan in Planning Center";
            return false;
        }
        
        // Extract plan ID from response and add items
        // This is a simplified implementation
        return true;
        
    } catch (const std::exception& e) {
        last_error_ = "Error exporting to Planning Center: " + std::string(e.what());
        return false;
    }
}

bool PlanningCenterProvider::importServicePlan(ServicePlan& plan, const IntegrationConfig& config) {
    try {
        // Get list of available services
        auto services = getAvailableServices(config);
        if (services.empty()) {
            last_error_ = "No services found in Planning Center";
            return false;
        }
        
        // For simplicity, import the first service found
        // In a real implementation, this would be user-selected
        std::string service_id = services[0];
        
        std::string response;
        bool success = makeApiRequest("/service_types/1/plans/" + service_id, "GET", "", response, config);
        
        if (!success) {
            last_error_ = "Failed to retrieve service plan from Planning Center";
            return false;
        }
        
        // Parse the response and populate the service plan
        // This is a simplified implementation
        plan.setTitle("Imported from Planning Center");
        plan.setDescription("Service plan imported from Planning Center Online");
        
        return true;
        
    } catch (const std::exception& e) {
        last_error_ = "Error importing from Planning Center: " + std::string(e.what());
        return false;
    }
}

std::string PlanningCenterProvider::generateOAuthUrl() const {
    std::ostringstream url;
    url << OAUTH_BASE_URL << "/authorize"
        << "?client_id=" << CLIENT_ID
        << "&redirect_uri=" << REDIRECT_URI
        << "&response_type=code"
        << "&scope=services";
    return url.str();
}

bool PlanningCenterProvider::handleOAuthCallback(const std::string& code, IntegrationConfig& config) {
    // Exchange authorization code for access token
    std::ostringstream body;
    body << "grant_type=authorization_code"
         << "&client_id=" << CLIENT_ID
         << "&client_secret=" << config.client_secret
         << "&redirect_uri=" << REDIRECT_URI
         << "&code=" << code;
    
    // In a real implementation, this would make an HTTP request to the token endpoint
    // For now, we'll simulate a successful token exchange
    config.api_key = "mock_access_token_" + code.substr(0, 8);
    
    return true;
}

std::string PlanningCenterProvider::getLastError() const {
    return last_error_;
}

bool PlanningCenterProvider::syncServiceOrders(const IntegrationConfig& config) {
    // Implementation for syncing service orders with Planning Center Live
    std::string response;
    return makeApiRequest("/service_types/1/plans?filter=future", "GET", "", response, config);
}

bool PlanningCenterProvider::importScriptureReadings(const IntegrationConfig& config, ServicePlan& plan) {
    // Get scripture readings from Planning Center
    std::string response;
    bool success = makeApiRequest("/service_types/1/plans?include=items", "GET", "", response, config);
    
    if (success) {
        // Parse response and extract scripture items
        // Add them to the service plan
        ServiceItem scripture_item;
        scripture_item.type = ServiceItemType::SCRIPTURE;
        scripture_item.title = "Scripture Reading";
        scripture_item.content = "John 3:16";
        scripture_item.translation = "ESV";
        plan.addItem(scripture_item);
    }
    
    return success;
}

bool PlanningCenterProvider::exportVerseCollections(const ServicePlan& plan, const IntegrationConfig& config) {
    // Export verse collections back to Planning Center
    auto scripture_items = plan.findItemsByType(ServiceItemType::SCRIPTURE);
    
    for (const auto& item : scripture_items) {
        std::ostringstream json;
        json << "{"
             << "\"data\": {"
             << "\"type\": \"Item\","
             << "\"attributes\": {"
             << "\"title\": \"" << item.title << "\","
             << "\"description\": \"" << item.content << "\","
             << "\"item_type\": \"song\""
             << "}"
             << "}"
             << "}";
        
        std::string response;
        makeApiRequest("/service_types/1/plans/1/items", "POST", json.str(), response, config);
    }
    
    return true;
}

std::vector<std::string> PlanningCenterProvider::getAvailableServices(const IntegrationConfig& config) {
    std::string response;
    bool success = makeApiRequest("/service_types/1/plans?filter=future&per_page=10", "GET", "", response, config);
    
    std::vector<std::string> service_ids;
    if (success) {
        // Parse JSON response and extract service IDs
        // This is a simplified implementation
        service_ids.push_back("12345");  // Mock service ID
        service_ids.push_back("12346");  // Mock service ID
    }
    
    return service_ids;
}

bool PlanningCenterProvider::makeApiRequest(const std::string& endpoint, const std::string& method,
                                           const std::string& body, std::string& response,
                                           const IntegrationConfig& config) {
    // In a real implementation, this would make actual HTTP requests
    // For now, we'll simulate API responses
    
    if (config.api_key.empty()) {
        last_error_ = "No API key configured";
        return false;
    }
    
    // Simulate successful response
    if (endpoint.find("/service_types") != std::string::npos) {
        response = R"({
            "data": [
                {
                    "type": "ServiceType",
                    "id": "1",
                    "attributes": {
                        "name": "Sunday Morning Service"
                    }
                }
            ]
        })";
        return true;
    }
    
    response = "{}";
    return true;
}

std::string PlanningCenterProvider::buildAuthHeader(const IntegrationConfig& config) const {
    return "Bearer " + config.api_key;
}

bool PlanningCenterProvider::refreshAccessToken(IntegrationConfig& config) {
    // Implementation for refreshing OAuth tokens
    return true;
}

std::vector<PlanningCenterProvider::PCOService> PlanningCenterProvider::parseServicesResponse(const std::string& response) {
    std::vector<PCOService> services;
    // Parse JSON response and extract service information
    return services;
}

std::vector<PlanningCenterProvider::PCOItem> PlanningCenterProvider::parseItemsResponse(const std::string& response) {
    std::vector<PCOItem> items;
    // Parse JSON response and extract item information
    return items;
}

ServicePlan PlanningCenterProvider::convertPCOToServicePlan(const PCOService& service, const std::vector<PCOItem>& items) {
    ServicePlan plan(service.name, service.service_time);
    plan.setDescription(service.series_title + " - " + service.plan_title);
    
    for (const auto& pco_item : items) {
        ServiceItem item;
        item.title = pco_item.title;
        item.description = pco_item.description;
        item.duration = pco_item.length;
        item.assigned_to = pco_item.assigned_to;
        
        // Map PCO categories to service item types
        if (pco_item.category == "song") {
            item.type = ServiceItemType::SONG;
        } else if (pco_item.category == "header") {
            item.type = ServiceItemType::CUSTOM;
        } else {
            item.type = ServiceItemType::CUSTOM;
        }
        
        plan.addItem(item);
    }
    
    return plan;
}

std::string PlanningCenterProvider::convertServicePlanToPCO(const ServicePlan& plan) {
    std::ostringstream json;
    json << "{"
         << "\"data\": {"
         << "\"type\": \"Plan\","
         << "\"attributes\": {"
         << "\"title\": \"" << plan.getTitle() << "\","
         << "\"series_title\": \"" << plan.getDescription() << "\""
         << "}"
         << "}"
         << "}";
    return json.str();
}