#include <iostream>
#include <iomanip>
#include "integrations/IntegrationManager.h"
#include "service/ServicePlan.h"

void printServicePlanDemo() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "           VERSEFINDER CHURCH INTEGRATION DEMO" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    // Demo service plan
    ServicePlan plan("Easter Sunday Service", std::chrono::system_clock::now());
    plan.setDescription("Special Easter celebration service");
    
    // Add various service items
    ServiceItem welcome;
    welcome.type = ServiceItemType::ANNOUNCEMENT;
    welcome.title = "Welcome & Announcements";
    welcome.content = "Easter greetings and church updates";
    welcome.duration = std::chrono::seconds(300); // 5 minutes
    plan.addItem(welcome);
    
    ServiceItem openingSong;
    openingSong.type = ServiceItemType::SONG;
    openingSong.title = "Christ the Lord is Risen Today";
    openingSong.content = "Traditional Easter hymn";
    openingSong.duration = std::chrono::seconds(240); // 4 minutes
    plan.addItem(openingSong);
    
    ServiceItem prayer;
    prayer.type = ServiceItemType::PRAYER;
    prayer.title = "Opening Prayer";
    prayer.content = "Prayer of thanksgiving for resurrection";
    prayer.duration = std::chrono::seconds(120); // 2 minutes
    plan.addItem(prayer);
    
    ServiceItem scripture;
    scripture.type = ServiceItemType::SCRIPTURE;
    scripture.title = "Easter Scripture Reading";
    scripture.content = "Matthew 28:1-10";
    scripture.book = "Matthew";
    scripture.chapter = 28;
    scripture.verse_start = 1;
    scripture.verse_end = 10;
    scripture.translation = "ESV";
    scripture.duration = std::chrono::seconds(180); // 3 minutes
    plan.addItem(scripture);
    
    ServiceItem sermon;
    sermon.type = ServiceItemType::SERMON;
    sermon.title = "He is Risen Indeed!";
    sermon.content = "Easter message on the power of resurrection";
    sermon.duration = std::chrono::seconds(1800); // 30 minutes
    plan.addItem(sermon);
    
    ServiceItem communion;
    communion.type = ServiceItemType::COMMUNION;
    communion.title = "Communion";
    communion.content = "Remembrance of Christ's sacrifice";
    communion.duration = std::chrono::seconds(600); // 10 minutes
    plan.addItem(communion);
    
    ServiceItem closingSong;
    closingSong.type = ServiceItemType::SONG;
    closingSong.title = "Amazing Grace";
    closingSong.content = "How sweet the sound";
    closingSong.duration = std::chrono::seconds(300); // 5 minutes
    plan.addItem(closingSong);
    
    // Display service plan
    std::cout << "\n📋 SERVICE PLAN: " << plan.getTitle() << std::endl;
    auto service_time_t = std::chrono::system_clock::to_time_t(plan.getServiceTime());
    std::cout << "📅 Service Time: " << std::put_time(std::gmtime(&service_time_t), "%Y-%m-%d %H:%M") << std::endl;
    std::cout << "📝 Description: " << plan.getDescription() << std::endl;
    std::cout << "⏱️  Total Duration: " << plan.getTotalDuration().count() / 60 << " minutes" << std::endl;
    std::cout << "\n" << std::string(60, '-') << std::endl;
    std::cout << "SERVICE ORDER:" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    int itemNum = 1;
    for (const auto& item : plan.getItems()) {
        std::string typeIcon;
        switch (item.type) {
            case ServiceItemType::SONG: typeIcon = "🎵"; break;
            case ServiceItemType::SCRIPTURE: typeIcon = "📖"; break;
            case ServiceItemType::SERMON: typeIcon = "🎤"; break;
            case ServiceItemType::PRAYER: typeIcon = "🙏"; break;
            case ServiceItemType::ANNOUNCEMENT: typeIcon = "📢"; break;
            case ServiceItemType::COMMUNION: typeIcon = "🍞"; break;
            default: typeIcon = "📄"; break;
        }
        
        std::cout << std::setw(2) << itemNum++ << ". " << typeIcon << " " 
                  << std::setw(25) << std::left << item.title 
                  << " (" << item.duration.count() / 60 << " min)" << std::endl;
        std::cout << "    📝 " << item.content << std::endl;
        if (item.type == ServiceItemType::SCRIPTURE) {
            std::cout << "    📖 " << item.book << " " << item.chapter 
                      << ":" << item.verse_start << "-" << item.verse_end 
                      << " (" << item.translation << ")" << std::endl;
        }
        std::cout << std::endl;
    }
}

void printIntegrationsDemo() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "          CHURCH MANAGEMENT INTEGRATIONS" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    IntegrationManager manager;
    auto integrations = manager.getAvailableIntegrations();
    
    std::cout << "\n🔗 Available Integrations (" << integrations.size() << "):\n" << std::endl;
    
    // Group integrations by type
    std::cout << "📋 CHURCH MANAGEMENT SYSTEMS:" << std::endl;
    std::cout << std::string(40, '-') << std::endl;
    for (const auto& integration : integrations) {
        if (integration.type <= IntegrationType::CHURCH_COMMUNITY_BUILDER) {
            std::string features;
            if (integration.supports_import) features += "📥 Import ";
            if (integration.supports_export) features += "📤 Export ";
            if (integration.supports_realtime) features += "🔄 Real-time ";
            if (integration.requires_oauth) features += "🔐 OAuth ";
            
            std::cout << "• " << std::setw(20) << std::left << integration.name 
                      << " - " << integration.description << std::endl;
            std::cout << "  " << features << std::endl << std::endl;
        }
    }
    
    std::cout << "\n🎬 WORSHIP SOFTWARE:" << std::endl;
    std::cout << std::string(40, '-') << std::endl;
    for (const auto& integration : integrations) {
        if (integration.type > IntegrationType::CHURCH_COMMUNITY_BUILDER) {
            std::string features;
            if (integration.supports_export) features += "📤 Export ";
            
            std::cout << "• " << std::setw(20) << std::left << integration.name 
                      << " - " << integration.description << std::endl;
            std::cout << "  " << features << std::endl << std::endl;
        }
    }
}

void printAPIDemo() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "               API ENDPOINTS AVAILABLE" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::cout << "\n🌐 REST API Endpoints:\n" << std::endl;
    
    std::cout << "GET    /api/service-plans           List all service plans" << std::endl;
    std::cout << "POST   /api/service-plans           Create new service plan" << std::endl;
    std::cout << "GET    /api/service-plans/{id}      Get specific service plan" << std::endl;
    std::cout << "PUT    /api/service-plans/{id}      Update service plan" << std::endl;
    std::cout << "DELETE /api/service-plans/{id}      Delete service plan" << std::endl;
    std::cout << std::endl;
    std::cout << "GET    /api/integrations            List available integrations" << std::endl;
    std::cout << "POST   /api/integrations/{type}     Configure integration" << std::endl;
    std::cout << "GET    /api/integrations/{type}     Get integration status" << std::endl;
    std::cout << "DELETE /api/integrations/{type}     Remove integration" << std::endl;
    std::cout << std::endl;
    std::cout << "POST   /api/sync/{type}             Sync with integration" << std::endl;
    std::cout << "POST   /api/export/{type}           Export service plan" << std::endl;
    std::cout << "POST   /api/import/{type}           Import service plan" << std::endl;
    
    std::cout << "\n🔗 Webhook Events:" << std::endl;
    std::cout << "• service_plan_created" << std::endl;
    std::cout << "• service_plan_updated" << std::endl;
    std::cout << "• integration_connected" << std::endl;
    std::cout << "• sync_completed" << std::endl;
}

int main() {
    printServicePlanDemo();
    printIntegrationsDemo();
    printAPIDemo();
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "     CHURCH MANAGEMENT INTEGRATION DEMO COMPLETE" << std::endl;
    std::cout << "               ✅ All Systems Operational" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    return 0;
}