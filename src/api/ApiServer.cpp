#include "ApiServer.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <unordered_set>
#include <algorithm>

// Private implementation details
struct ApiServer::Impl {
    bool running = false;
    int port = 8080;
    std::unordered_map<std::string, ApiHandler> routes;
    std::vector<std::function<bool(ApiRequest&, ApiResponse&)>> middlewares;
    std::function<bool(const std::string&, std::string&)> auth_handler;
    std::unordered_set<std::string> protected_paths;
    std::unordered_map<std::string, RateLimit> rate_limits;
    RateLimit global_rate_limit;
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::chrono::steady_clock::time_point>>> rate_limit_tracker;
    bool cors_enabled = false;
    std::string cors_origins = "*";
    std::unordered_map<std::string, std::string> cors_headers;
    std::unordered_map<std::string, std::vector<std::string>> webhooks;
    std::function<ApiResponse(int, const std::string&)> error_handler;
    std::function<void(const std::string&)> log_handler;
    std::string log_level = "INFO";
};

ApiServer::ApiServer() : impl_(std::make_unique<Impl>()) {
    // Set default error handler
    impl_->error_handler = [](int status, const std::string& message) {
        return errorResponse(status, message);
    };
    
    // Set default log handler
    impl_->log_handler = [](const std::string& message) {
        std::cout << "[API] " << message << std::endl;
    };
}

ApiServer::~ApiServer() {
    stop();
}

bool ApiServer::start(int port) {
    if (impl_->running) {
        return false;
    }
    
    impl_->port = port;
    impl_->running = true;
    
    // In a real implementation, this would start an actual HTTP server
    // For now, we'll just log that the server is starting
    if (impl_->log_handler) {
        impl_->log_handler("API Server starting on port " + std::to_string(port));
    }
    
    return true;
}

void ApiServer::stop() {
    if (impl_->running) {
        impl_->running = false;
        if (impl_->log_handler) {
            impl_->log_handler("API Server stopped");
        }
    }
}

bool ApiServer::isRunning() const {
    return impl_->running;
}

void ApiServer::addRoute(HttpMethod method, const std::string& path, ApiHandler handler) {
    std::string method_str;
    switch (method) {
        case HttpMethod::GET: method_str = "GET"; break;
        case HttpMethod::POST: method_str = "POST"; break;
        case HttpMethod::PUT: method_str = "PUT"; break;
        case HttpMethod::DELETE: method_str = "DELETE"; break;
        case HttpMethod::PATCH: method_str = "PATCH"; break;
    }
    
    std::string route_key = method_str + " " + path;
    impl_->routes[route_key] = handler;
    
    if (impl_->log_handler) {
        impl_->log_handler("Route added: " + route_key);
    }
}

void ApiServer::addMiddleware(std::function<bool(ApiRequest&, ApiResponse&)> middleware) {
    impl_->middlewares.push_back(middleware);
}

void ApiServer::setAuthHandler(std::function<bool(const std::string& token, std::string& user_id)> auth_handler) {
    impl_->auth_handler = auth_handler;
}

void ApiServer::requireAuth(const std::string& path) {
    impl_->protected_paths.insert(path);
}

void ApiServer::setRateLimit(const std::string& path, const RateLimit& limit) {
    impl_->rate_limits[path] = limit;
}

void ApiServer::setGlobalRateLimit(const RateLimit& limit) {
    impl_->global_rate_limit = limit;
}

void ApiServer::enableCors(const std::string& allowed_origins) {
    impl_->cors_enabled = true;
    impl_->cors_origins = allowed_origins;
}

void ApiServer::setCorsHeaders(const std::unordered_map<std::string, std::string>& headers) {
    impl_->cors_headers = headers;
}

void ApiServer::addWebhook(const std::string& event, const std::string& url) {
    impl_->webhooks[event].push_back(url);
}

void ApiServer::removeWebhook(const std::string& event, const std::string& url) {
    auto& urls = impl_->webhooks[event];
    urls.erase(std::remove(urls.begin(), urls.end(), url), urls.end());
}

void ApiServer::triggerWebhook(const std::string& event, const std::string& payload) {
    auto it = impl_->webhooks.find(event);
    if (it != impl_->webhooks.end()) {
        for (const auto& url : it->second) {
            // In a real implementation, this would make HTTP POST requests to the webhook URLs
            if (impl_->log_handler) {
                impl_->log_handler("Triggering webhook: " + event + " -> " + url);
            }
        }
    }
}

void ApiServer::setErrorHandler(std::function<ApiResponse(int status, const std::string& message)> handler) {
    impl_->error_handler = handler;
}

void ApiServer::setLogLevel(const std::string& level) {
    impl_->log_level = level;
}

void ApiServer::setLogHandler(std::function<void(const std::string& message)> handler) {
    impl_->log_handler = handler;
}

bool ApiServer::checkRateLimit(const std::string& client_ip, const std::string& path) {
    auto now = std::chrono::steady_clock::now();
    auto& client_requests = impl_->rate_limit_tracker[client_ip][path];
    
    // Clean old requests (older than 1 day)
    auto day_ago = now - std::chrono::hours(24);
    client_requests.erase(
        std::remove_if(client_requests.begin(), client_requests.end(),
            [day_ago](const auto& time) { return time < day_ago; }),
        client_requests.end()
    );
    
    // Check rate limits
    RateLimit limit = impl_->global_rate_limit;
    auto it = impl_->rate_limits.find(path);
    if (it != impl_->rate_limits.end()) {
        limit = it->second;
    }
    
    auto minute_ago = now - std::chrono::minutes(1);
    auto hour_ago = now - std::chrono::hours(1);
    
    int requests_last_minute = std::count_if(client_requests.begin(), client_requests.end(),
        [minute_ago](const auto& time) { return time > minute_ago; });
    int requests_last_hour = std::count_if(client_requests.begin(), client_requests.end(),
        [hour_ago](const auto& time) { return time > hour_ago; });
    int requests_last_day = client_requests.size();
    
    if (requests_last_minute >= limit.requests_per_minute ||
        requests_last_hour >= limit.requests_per_hour ||
        requests_last_day >= limit.requests_per_day) {
        return false;
    }
    
    client_requests.push_back(now);
    return true;
}

bool ApiServer::authenticate(ApiRequest& request) {
    // Check if path requires authentication
    bool requires_auth = false;
    for (const auto& protected_path : impl_->protected_paths) {
        if (request.path.find(protected_path) == 0) {
            requires_auth = true;
            break;
        }
    }
    
    if (!requires_auth) {
        return true;
    }
    
    if (!impl_->auth_handler) {
        return false;
    }
    
    auto auth_header = request.headers.find("Authorization");
    if (auth_header == request.headers.end()) {
        return false;
    }
    
    std::string token = auth_header->second;
    if (token.substr(0, 7) == "Bearer ") {
        token = token.substr(7);
    }
    
    return impl_->auth_handler(token, request.user_id);
}

ApiResponse ApiServer::handleRequest(const ApiRequest& request) {
    // Apply middlewares
    ApiRequest mutable_request = request;
    ApiResponse response;
    
    for (auto& middleware : impl_->middlewares) {
        if (!middleware(mutable_request, response)) {
            return response;  // Middleware rejected the request
        }
    }
    
    // Check rate limiting
    if (!checkRateLimit(request.client_ip, request.path)) {
        return impl_->error_handler(429, "Rate limit exceeded");
    }
    
    // Authenticate
    if (!authenticate(mutable_request)) {
        return impl_->error_handler(401, "Unauthorized");
    }
    
    // Find and execute handler
    std::string method_str;
    switch (request.method) {
        case HttpMethod::GET: method_str = "GET"; break;
        case HttpMethod::POST: method_str = "POST"; break;
        case HttpMethod::PUT: method_str = "PUT"; break;
        case HttpMethod::DELETE: method_str = "DELETE"; break;
        case HttpMethod::PATCH: method_str = "PATCH"; break;
    }
    
    std::string route_key = method_str + " " + request.path;
    auto it = impl_->routes.find(route_key);
    
    if (it != impl_->routes.end()) {
        try {
            response = it->second(mutable_request);
        } catch (const std::exception& e) {
            response = impl_->error_handler(500, "Internal server error: " + std::string(e.what()));
        }
    } else {
        response = impl_->error_handler(404, "Not found");
    }
    
    // Add CORS headers if enabled
    if (impl_->cors_enabled) {
        response.headers["Access-Control-Allow-Origin"] = impl_->cors_origins;
        response.headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, PATCH, OPTIONS";
        response.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
        
        for (const auto& header : impl_->cors_headers) {
            response.headers[header.first] = header.second;
        }
    }
    
    return response;
}

void ApiServer::logRequest(const ApiRequest& request, const ApiResponse& response) {
    if (impl_->log_handler) {
        std::string method_str;
        switch (request.method) {
            case HttpMethod::GET: method_str = "GET"; break;
            case HttpMethod::POST: method_str = "POST"; break;
            case HttpMethod::PUT: method_str = "PUT"; break;
            case HttpMethod::DELETE: method_str = "DELETE"; break;
            case HttpMethod::PATCH: method_str = "PATCH"; break;
        }
        
        std::string log_message = request.client_ip + " " + method_str + " " + 
                                 request.path + " " + std::to_string(response.status_code);
        impl_->log_handler(log_message);
    }
}

// Utility functions
ApiResponse jsonResponse(const std::string& json, int status) {
    ApiResponse response;
    response.status_code = status;
    response.headers["Content-Type"] = "application/json";
    response.body = json;
    return response;
}

ApiResponse errorResponse(int status, const std::string& message) {
    std::string json = "{\"error\": \"" + message + "\"}";
    return jsonResponse(json, status);
}

ApiResponse successResponse(const std::string& message) {
    std::string json = "{\"message\": \"" + message + "\"}";
    return jsonResponse(json, 200);
}