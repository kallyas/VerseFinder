#ifndef INTEGRATION_PROVIDER_H
#define INTEGRATION_PROVIDER_H

#include <string>
#include "IntegrationManager.h"

class ServicePlan;

class IntegrationProvider {
public:
    virtual ~IntegrationProvider() = default;
    
    virtual bool testConnection(const IntegrationConfig& config) = 0;
    virtual bool exportServicePlan(const ServicePlan& plan, const IntegrationConfig& config) = 0;
    virtual bool importServicePlan(ServicePlan& plan, const IntegrationConfig& config) = 0;
    virtual std::string generateOAuthUrl() const = 0;
    virtual bool handleOAuthCallback(const std::string& code, IntegrationConfig& config) = 0;
    virtual std::string getLastError() const = 0;
    
protected:
    mutable std::string last_error_;
};

#endif // INTEGRATION_PROVIDER_H