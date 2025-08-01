#include "IntegrationManager.h"
#include "IntegrationProvider.h"
#include "../service/ServicePlan.h"
#include <algorithm>

IntegrationManager::IntegrationManager() {
    initializeProviders();
}

IntegrationManager::~IntegrationManager() = default;

bool IntegrationManager::addIntegration(const IntegrationConfig& config) {
    auto it = providers_.find(config.type);
    if (it == providers_.end()) {
        errors_[config.type] = "Provider not found for integration type";
        return false;
    }
    
    configs_[config.type] = config;
    updateStatus(config.type, IntegrationStatus::DISCONNECTED);
    return true;
}

bool IntegrationManager::removeIntegration(IntegrationType type) {
    auto it = configs_.find(type);
    if (it == configs_.end()) {
        return false;
    }
    
    configs_.erase(it);
    statuses_.erase(type);
    errors_.erase(type);
    return true;
}

bool IntegrationManager::testConnection(IntegrationType type) {
    auto config_it = configs_.find(type);
    auto provider_it = providers_.find(type);
    
    if (config_it == configs_.end() || provider_it == providers_.end()) {
        return false;
    }
    
    updateStatus(type, IntegrationStatus::CONNECTING);
    
    bool success = provider_it->second->testConnection(config_it->second);
    updateStatus(type, success ? IntegrationStatus::CONNECTED : IntegrationStatus::ERROR);
    
    if (!success) {
        errors_[type] = provider_it->second->getLastError();
    }
    
    return success;
}

IntegrationStatus IntegrationManager::getStatus(IntegrationType type) const {
    auto it = statuses_.find(type);
    return it != statuses_.end() ? it->second : IntegrationStatus::DISCONNECTED;
}

bool IntegrationManager::exportServicePlan(const ServicePlan& plan, IntegrationType target) {
    auto config_it = configs_.find(target);
    auto provider_it = providers_.find(target);
    
    if (config_it == configs_.end() || provider_it == providers_.end()) {
        return false;
    }
    
    if (getStatus(target) != IntegrationStatus::CONNECTED) {
        errors_[target] = "Integration not connected";
        return false;
    }
    
    updateStatus(target, IntegrationStatus::SYNCING);
    bool success = provider_it->second->exportServicePlan(plan, config_it->second);
    updateStatus(target, success ? IntegrationStatus::CONNECTED : IntegrationStatus::ERROR);
    
    if (!success) {
        errors_[target] = provider_it->second->getLastError();
    }
    
    return success;
}

bool IntegrationManager::importServicePlan(IntegrationType source, ServicePlan& plan) {
    auto config_it = configs_.find(source);
    auto provider_it = providers_.find(source);
    
    if (config_it == configs_.end() || provider_it == providers_.end()) {
        return false;
    }
    
    if (getStatus(source) != IntegrationStatus::CONNECTED) {
        errors_[source] = "Integration not connected";
        return false;
    }
    
    updateStatus(source, IntegrationStatus::SYNCING);
    bool success = provider_it->second->importServicePlan(plan, config_it->second);
    updateStatus(source, success ? IntegrationStatus::CONNECTED : IntegrationStatus::ERROR);
    
    if (!success) {
        errors_[source] = provider_it->second->getLastError();
    }
    
    return success;
}

bool IntegrationManager::syncServicePlans(IntegrationType type) {
    // Placeholder for two-way sync implementation
    return false;
}

void IntegrationManager::enableRealTimeSync(IntegrationType type, bool enable) {
    auto config_it = configs_.find(type);
    if (config_it != configs_.end()) {
        config_it->second.auto_sync = enable;
    }
}

void IntegrationManager::setStatusCallback(std::function<void(IntegrationType, IntegrationStatus)> callback) {
    status_callback_ = callback;
}

std::vector<IntegrationInfo> IntegrationManager::getAvailableIntegrations() const {
    return {
        {IntegrationType::PLANNING_CENTER, "Planning Center", "Planning Center Online integration", "", true, true, true, true},
        {IntegrationType::CHURCH_TOOLS, "ChurchTools", "ChurchTools management system", "", true, true, true, false},
        {IntegrationType::BREEZE_CHMS, "Breeze ChMS", "Breeze Church Management System", "", false, true, false, false},
        {IntegrationType::ROCK_RMS, "Rock RMS", "Rock Relationship Management System", "", true, true, true, false},
        {IntegrationType::ELVANTO, "Elvanto", "Elvanto/PushPay integration", "", true, true, true, false},
        {IntegrationType::CHURCH_COMMUNITY_BUILDER, "CCB", "Church Community Builder", "", true, true, false, false},
        {IntegrationType::PROPRESENTER, "ProPresenter", "ProPresenter slide export", "", false, true, false, false},
        {IntegrationType::EASY_WORSHIP, "EasyWorship", "EasyWorship compatibility", "", false, true, false, false},
        {IntegrationType::MEDIA_SHOUT, "MediaShout", "MediaShout integration", "", false, true, false, false},
        {IntegrationType::OPEN_LP, "OpenLP", "OpenLP plugin development", "", false, true, false, false},
        {IntegrationType::PROCLAIM, "Proclaim", "Proclaim by Faithlife", "", true, true, false, false}
    };
}

std::vector<IntegrationType> IntegrationManager::getActiveIntegrations() const {
    std::vector<IntegrationType> active;
    for (const auto& pair : configs_) {
        active.push_back(pair.first);
    }
    return active;
}

IntegrationConfig IntegrationManager::getConfig(IntegrationType type) const {
    auto it = configs_.find(type);
    return it != configs_.end() ? it->second : IntegrationConfig{};
}

bool IntegrationManager::updateConfig(const IntegrationConfig& config) {
    configs_[config.type] = config;
    return true;
}

std::string IntegrationManager::generateOAuthUrl(IntegrationType type) const {
    auto provider_it = providers_.find(type);
    if (provider_it != providers_.end()) {
        return provider_it->second->generateOAuthUrl();
    }
    return "";
}

bool IntegrationManager::handleOAuthCallback(IntegrationType type, const std::string& code) {
    auto provider_it = providers_.find(type);
    auto config_it = configs_.find(type);
    
    if (provider_it != providers_.end() && config_it != configs_.end()) {
        return provider_it->second->handleOAuthCallback(code, config_it->second);
    }
    return false;
}

std::string IntegrationManager::getLastError(IntegrationType type) const {
    auto it = errors_.find(type);
    return it != errors_.end() ? it->second : "";
}

void IntegrationManager::clearErrors(IntegrationType type) {
    errors_.erase(type);
}

void IntegrationManager::initializeProviders() {
    // Initialize providers for each integration type
    // This would be expanded to include actual provider implementations
}

void IntegrationManager::updateStatus(IntegrationType type, IntegrationStatus status) {
    statuses_[type] = status;
    if (status_callback_) {
        status_callback_(type, status);
    }
}

std::string IntegrationManager::typeToString(IntegrationType type) const {
    switch (type) {
        case IntegrationType::PLANNING_CENTER: return "Planning Center";
        case IntegrationType::CHURCH_TOOLS: return "ChurchTools";
        case IntegrationType::BREEZE_CHMS: return "Breeze ChMS";
        case IntegrationType::ROCK_RMS: return "Rock RMS";
        case IntegrationType::ELVANTO: return "Elvanto";
        case IntegrationType::CHURCH_COMMUNITY_BUILDER: return "CCB";
        case IntegrationType::PROPRESENTER: return "ProPresenter";
        case IntegrationType::EASY_WORSHIP: return "EasyWorship";
        case IntegrationType::MEDIA_SHOUT: return "MediaShout";
        case IntegrationType::OPEN_LP: return "OpenLP";
        case IntegrationType::PROCLAIM: return "Proclaim";
        default: return "Unknown";
    }
}