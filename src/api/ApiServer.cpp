#include "ApiServer.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <atomic>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <csignal>
#include <iomanip>

// Private implementation details
struct ApiServer::Impl {
    std::atomic<bool> running{false};
    int port = 8080;
    int server_socket = -1;
    std::thread server_thread;
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
    
    void serverLoop(ApiServer* server);
    std::string parseHttpRequest(const std::string& raw_request, ApiRequest& request);
    std::string formatHttpResponse(const ApiResponse& response);
    std::string urlDecode(const std::string& encoded);
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
    if (impl_->running.load()) {
        return false;
    }
    
    impl_->port = port;
    
    // Create socket
    impl_->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (impl_->server_socket < 0) {
        if (impl_->log_handler) {
            impl_->log_handler("Failed to create socket");
        }
        return false;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(impl_->server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(impl_->server_socket);
        impl_->server_socket = -1;
        return false;
    }
    
    // Bind socket
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    if (bind(impl_->server_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        if (impl_->log_handler) {
            impl_->log_handler("Failed to bind to port " + std::to_string(port));
        }
        close(impl_->server_socket);
        impl_->server_socket = -1;
        return false;
    }
    
    // Start listening
    if (listen(impl_->server_socket, 10) < 0) {
        if (impl_->log_handler) {
            impl_->log_handler("Failed to listen on socket");
        }
        close(impl_->server_socket);
        impl_->server_socket = -1;
        return false;
    }
    
    impl_->running.store(true);
    
    // Start server thread
    impl_->server_thread = std::thread(&ApiServer::Impl::serverLoop, impl_.get(), this);
    
    if (impl_->log_handler) {
        impl_->log_handler("API Server started on port " + std::to_string(port));
    }
    
    return true;
}

void ApiServer::stop() {
    if (impl_->running.load()) {
        impl_->running.store(false);
        
        // Close server socket to break accept() loop
        if (impl_->server_socket >= 0) {
            close(impl_->server_socket);
            impl_->server_socket = -1;
        }
        
        // Wait for server thread to finish
        if (impl_->server_thread.joinable()) {
            impl_->server_thread.join();
        }
        
        if (impl_->log_handler) {
            impl_->log_handler("API Server stopped");
        }
    }
}

bool ApiServer::isRunning() const {
    return impl_->running.load();
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
    // Suppress unused parameter warning for mock implementation
    (void)payload;
    
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

// Implementation of server loop and HTTP parsing
void ApiServer::Impl::serverLoop(ApiServer* server) {
    while (running.load()) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_socket < 0) {
            if (running.load()) {
                // Only log if we're still supposed to be running
                if (log_handler) {
                    log_handler("Accept failed");
                }
            }
            continue;
        }
        
        // Handle request in a separate thread for better concurrency
        std::thread([this, client_socket, server, client_addr]() {
            char buffer[4096];
            ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                std::string raw_request(buffer);
                
                ApiRequest request;
                request.client_ip = inet_ntoa(client_addr.sin_addr);
                
                std::string parse_error = parseHttpRequest(raw_request, request);
                ApiResponse response;
                
                if (!parse_error.empty()) {
                    response = server->impl_->error_handler(400, "Bad Request: " + parse_error);
                } else {
                    response = server->handleRequest(request);
                }
                
                std::string http_response = formatHttpResponse(response);
                send(client_socket, http_response.c_str(), http_response.length(), 0);
                
                server->logRequest(request, response);
            }
            
            close(client_socket);
        }).detach();
    }
}

std::string ApiServer::Impl::parseHttpRequest(const std::string& raw_request, ApiRequest& request) {
    std::istringstream stream(raw_request);
    std::string line;
    
    // Parse request line
    if (!std::getline(stream, line)) {
        return "Missing request line";
    }
    
    std::istringstream request_line(line);
    std::string method_str, path, version;
    request_line >> method_str >> path >> version;
    
    // Parse method
    if (method_str == "GET") request.method = HttpMethod::GET;
    else if (method_str == "POST") request.method = HttpMethod::POST;
    else if (method_str == "PUT") request.method = HttpMethod::PUT;
    else if (method_str == "DELETE") request.method = HttpMethod::DELETE;
    else if (method_str == "PATCH") request.method = HttpMethod::PATCH;
    else return "Unsupported method: " + method_str;
    
    // Parse path and query parameters
    size_t query_pos = path.find('?');
    if (query_pos != std::string::npos) {
        std::string query_string = path.substr(query_pos + 1);
        path = path.substr(0, query_pos);
        
        // Parse query parameters with URL decoding
        std::istringstream query_stream(query_string);
        std::string param;
        while (std::getline(query_stream, param, '&')) {
            size_t eq_pos = param.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = param.substr(0, eq_pos);
                std::string value = param.substr(eq_pos + 1);
                
                // URL decode both key and value
                key = this->urlDecode(key);
                value = this->urlDecode(value);
                
                request.query_params[key] = value;
            }
        }
    }
    
    request.path = path;
    
    // Parse headers
    while (std::getline(stream, line) && line != "\r" && !line.empty()) {
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string header_name = line.substr(0, colon_pos);
            std::string header_value = line.substr(colon_pos + 1);
            
            // Trim whitespace
            header_value.erase(0, header_value.find_first_not_of(" \t"));
            header_value.erase(header_value.find_last_not_of(" \t\r") + 1);
            
            request.headers[header_name] = header_value;
        }
    }
    
    // Parse body (if present)
    std::string body_line;
    while (std::getline(stream, body_line)) {
        request.body += body_line + "\n";
    }
    
    if (!request.body.empty()) {
        request.body.pop_back(); // Remove last newline
    }
    
    return ""; // Success
}

std::string ApiServer::Impl::formatHttpResponse(const ApiResponse& response) {
    std::ostringstream http_response;
    
    http_response << "HTTP/1.1 " << response.status_code << " ";
    
    // Status text
    switch (response.status_code) {
        case 200: http_response << "OK"; break;
        case 400: http_response << "Bad Request"; break;
        case 401: http_response << "Unauthorized"; break;
        case 404: http_response << "Not Found"; break;
        case 429: http_response << "Too Many Requests"; break;
        case 500: http_response << "Internal Server Error"; break;
        default: http_response << "Unknown"; break;
    }
    
    http_response << "\r\n";
    
    // Headers
    for (const auto& header : response.headers) {
        http_response << header.first << ": " << header.second << "\r\n";
    }
    
    http_response << "Content-Length: " << response.body.length() << "\r\n";
    http_response << "\r\n";
    http_response << response.body;
    
    return http_response.str();
}

// URL decoding utility function
// Handles percent-encoded characters like %20 (space), %3A (:), etc.
// Also converts '+' to space as per HTML form encoding standard
std::string ApiServer::Impl::urlDecode(const std::string& encoded) {
    std::string decoded;
    decoded.reserve(encoded.length());
    
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            // Get the two hex digits after %
            std::string hex = encoded.substr(i + 1, 2);
            
            // Convert hex to integer
            char* end;
            unsigned long value = std::strtoul(hex.c_str(), &end, 16);
            
            // If conversion was successful and within valid range
            if (*end == '\0' && value <= 255) {
                decoded += static_cast<char>(value);
                i += 2; // Skip the two hex digits
            } else {
                // Invalid hex sequence, keep the % as is
                decoded += encoded[i];
            }
        } else if (encoded[i] == '+') {
            // '+' is encoded space in query parameters
            decoded += ' ';
        } else {
            decoded += encoded[i];
        }
    }
    
    return decoded;
}