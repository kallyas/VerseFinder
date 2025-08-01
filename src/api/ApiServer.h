#ifndef API_SERVER_H
#define API_SERVER_H

#include <string>
#include <functional>
#include <unordered_map>
#include <memory>

enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH
};

struct ApiRequest {
    HttpMethod method;
    std::string path;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> query_params;
    std::string body;
    std::string client_ip;
    std::string user_id;  // Populated after authentication
};

struct ApiResponse {
    int status_code;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    
    ApiResponse() : status_code(200) {
        headers["Content-Type"] = "application/json";
    }
};

using ApiHandler = std::function<ApiResponse(const ApiRequest&)>;

struct RateLimit {
    int requests_per_minute;
    int requests_per_hour;
    int requests_per_day;
    
    RateLimit() : requests_per_minute(60), requests_per_hour(1000), requests_per_day(10000) {}
};

class ApiServer {
public:
    ApiServer();
    ~ApiServer();
    
    // Server lifecycle
    bool start(int port = 8080);
    void stop();
    bool isRunning() const;
    
    // Route registration
    void addRoute(HttpMethod method, const std::string& path, ApiHandler handler);
    void addMiddleware(std::function<bool(ApiRequest&, ApiResponse&)> middleware);
    
    // Authentication
    void setAuthHandler(std::function<bool(const std::string& token, std::string& user_id)> auth_handler);
    void requireAuth(const std::string& path);
    
    // Rate limiting
    void setRateLimit(const std::string& path, const RateLimit& limit);
    void setGlobalRateLimit(const RateLimit& limit);
    
    // CORS configuration
    void enableCors(const std::string& allowed_origins = "*");
    void setCorsHeaders(const std::unordered_map<std::string, std::string>& headers);
    
    // Webhook support
    void addWebhook(const std::string& event, const std::string& url);
    void removeWebhook(const std::string& event, const std::string& url);
    void triggerWebhook(const std::string& event, const std::string& payload);
    
    // Error handling
    void setErrorHandler(std::function<ApiResponse(int status, const std::string& message)> handler);
    
    // Logging
    void setLogLevel(const std::string& level);
    void setLogHandler(std::function<void(const std::string& message)> handler);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    bool checkRateLimit(const std::string& client_ip, const std::string& path);
    bool authenticate(ApiRequest& request);
    ApiResponse handleRequest(const ApiRequest& request);
    void logRequest(const ApiRequest& request, const ApiResponse& response);
};

// Utility functions for common API responses
ApiResponse jsonResponse(const std::string& json, int status = 200);
ApiResponse errorResponse(int status, const std::string& message);
ApiResponse successResponse(const std::string& message = "Success");

#endif // API_SERVER_H