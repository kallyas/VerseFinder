#include <iostream>
#include "../src/integrations/IntegrationManager.h"
#include "../src/service/ServicePlan.h"
#include "../src/api/ApiServer.h"

int main() {
    std::cout << "Testing VerseFinder Church Management Integration..." << std::endl;
    
    // Test IntegrationManager
    IntegrationManager manager;
    auto integrations = manager.getAvailableIntegrations();
    std::cout << "Available integrations: " << integrations.size() << std::endl;
    
    for (const auto& integration : integrations) {
        std::cout << "- " << integration.name << ": " << integration.description << std::endl;
    }
    
    // Test ServicePlan
    ServicePlan plan("Sunday Morning Service", std::chrono::system_clock::now());
    plan.setDescription("Test service plan for integration demo");
    
    ServiceItem scripture;
    scripture.type = ServiceItemType::SCRIPTURE;
    scripture.title = "Opening Scripture";
    scripture.content = "John 3:16";
    plan.addItem(scripture);
    
    ServiceItem song;
    song.type = ServiceItemType::SONG;
    song.title = "Amazing Grace";
    song.content = "Traditional hymn";
    plan.addItem(song);
    
    std::cout << "\nService Plan: " << plan.getTitle() << std::endl;
    std::cout << "Items: " << plan.getItems().size() << std::endl;
    
    // Test API Server
    ApiServer api;
    std::cout << "\nAPI Server initialized" << std::endl;
    
    // Test JSON export
    std::string exported = plan.exportToJson();
    std::cout << "\nExported service plan:\n" << exported.substr(0, 200) << "..." << std::endl;
    
    std::cout << "\nIntegration test completed successfully!" << std::endl;
    return 0;
}