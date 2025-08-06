#include "WebSocketServer.h"
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
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <iomanip>
#include <random>

// Private implementation details
struct WebSocketServer::Impl {
    std::atomic<bool> running{false};
    int port = 8081;
    int server_socket = -1;
    std::thread server_thread;
    std::thread ping_thread;
    std::atomic<bool> ping_running{false};
    
    mutable std::mutex connections_mutex;
    std::unordered_map<std::string, std::unique_ptr<WebSocketConnection>> connections;
    std::unordered_map<std::string, WebSocketHandler> message_handlers;
    std::function<bool(const std::string&, std::string&)> auth_handler;
    
    void serverLoop(WebSocketServer* server);
    void pingPongLoop(WebSocketServer* server);
};

WebSocketServer::WebSocketServer() : impl_(std::make_unique<Impl>()) {}

WebSocketServer::~WebSocketServer() {
    stop();
}

bool WebSocketServer::start(int port) {
    if (impl_->running.load()) {
        return false;
    }
    
    impl_->port = port;
    
    // Create socket
    impl_->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (impl_->server_socket < 0) {
        std::cerr << "Failed to create WebSocket server socket" << std::endl;
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
        std::cerr << "Failed to bind WebSocket server to port " << port << std::endl;
        close(impl_->server_socket);
        impl_->server_socket = -1;
        return false;
    }
    
    // Start listening
    if (listen(impl_->server_socket, 10) < 0) {
        std::cerr << "Failed to listen on WebSocket server socket" << std::endl;
        close(impl_->server_socket);
        impl_->server_socket = -1;
        return false;
    }
    
    impl_->running.store(true);
    
    // Start server thread
    impl_->server_thread = std::thread(&WebSocketServer::Impl::serverLoop, impl_.get(), this);
    
    std::cout << "WebSocket Server started on port " << port << std::endl;
    return true;
}

void WebSocketServer::stop() {
    if (impl_->running.load()) {
        impl_->running.store(false);
        
        // Stop ping/pong
        stopPingPong();
        
        // Close all connections
        {
            std::lock_guard<std::mutex> lock(impl_->connections_mutex);
            for (auto& pair : impl_->connections) {
                close(pair.second->socket_fd);
            }
            impl_->connections.clear();
        }
        
        // Close server socket
        if (impl_->server_socket >= 0) {
            close(impl_->server_socket);
            impl_->server_socket = -1;
        }
        
        // Wait for threads to finish
        if (impl_->server_thread.joinable()) {
            impl_->server_thread.join();
        }
        
        std::cout << "WebSocket Server stopped" << std::endl;
    }
}

bool WebSocketServer::isRunning() const {
    return impl_->running.load();
}

void WebSocketServer::setMessageHandler(const std::string& event_type, WebSocketHandler handler) {
    impl_->message_handlers[event_type] = handler;
}

void WebSocketServer::broadcastMessage(const std::string& event, const std::string& data) {
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    for (const auto& pair : impl_->connections) {
        const auto& connection = pair.second;
        if (connection->authenticated && 
            (connection->subscribed_events.empty() || 
             connection->subscribed_events.count(event))) {
            sendToConnection(pair.first, event, data);
        }
    }
}

void WebSocketServer::sendToConnection(const std::string& connection_id, const std::string& event, const std::string& data) {
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    auto it = impl_->connections.find(connection_id);
    if (it != impl_->connections.end()) {
        WebSocketMessage msg;
        msg.type = "event";
        msg.event = event;
        msg.data = data;
        
        // Create JSON message
        std::string json_msg = "{\"type\":\"" + msg.type + "\",\"event\":\"" + msg.event + "\",\"data\":" + msg.data + "}";
        
        auto frame = createWebSocketFrame(json_msg);
        send(it->second->socket_fd, frame.data(), frame.size(), 0);
    }
}

void WebSocketServer::sendToUser(const std::string& user_id, const std::string& event, const std::string& data) {
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    for (const auto& pair : impl_->connections) {
        if (pair.second->user_id == user_id && pair.second->authenticated) {
            sendToConnection(pair.first, event, data);
        }
    }
}

std::vector<std::string> WebSocketServer::getActiveConnections() const {
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    std::vector<std::string> connections;
    for (const auto& pair : impl_->connections) {
        if (pair.second->authenticated) {
            connections.push_back(pair.first);
        }
    }
    return connections;
}

std::vector<std::string> WebSocketServer::getConnectedUsers() const {
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    std::unordered_set<std::string> users;
    for (const auto& pair : impl_->connections) {
        if (pair.second->authenticated && !pair.second->user_id.empty()) {
            users.insert(pair.second->user_id);
        }
    }
    return std::vector<std::string>(users.begin(), users.end());
}

bool WebSocketServer::isUserConnected(const std::string& user_id) const {
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    for (const auto& pair : impl_->connections) {
        if (pair.second->user_id == user_id && pair.second->authenticated) {
            return true;
        }
    }
    return false;
}

void WebSocketServer::disconnectUser(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    std::vector<std::string> to_remove;
    for (const auto& pair : impl_->connections) {
        if (pair.second->user_id == user_id) {
            close(pair.second->socket_fd);
            to_remove.push_back(pair.first);
        }
    }
    for (const auto& conn_id : to_remove) {
        impl_->connections.erase(conn_id);
    }
}

void WebSocketServer::setAuthHandler(std::function<bool(const std::string& token, std::string& user_id)> auth_handler) {
    impl_->auth_handler = auth_handler;
}

bool WebSocketServer::authenticateConnection(const std::string& connection_id, const std::string& token) {
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    auto it = impl_->connections.find(connection_id);
    if (it != impl_->connections.end() && impl_->auth_handler) {
        std::string user_id;
        if (impl_->auth_handler(token, user_id)) {
            it->second->authenticated = true;
            it->second->user_id = user_id;
            return true;
        }
    }
    return false;
}

void WebSocketServer::subscribeToEvent(const std::string& connection_id, const std::string& event) {
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    auto it = impl_->connections.find(connection_id);
    if (it != impl_->connections.end()) {
        it->second->subscribed_events.insert(event);
    }
}

void WebSocketServer::unsubscribeFromEvent(const std::string& connection_id, const std::string& event) {
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    auto it = impl_->connections.find(connection_id);
    if (it != impl_->connections.end()) {
        it->second->subscribed_events.erase(event);
    }
}

void WebSocketServer::setDeviceName(const std::string& connection_id, const std::string& device_name) {
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    auto it = impl_->connections.find(connection_id);
    if (it != impl_->connections.end()) {
        it->second->device_name = device_name;
    }
}

std::string WebSocketServer::getDeviceName(const std::string& connection_id) const {
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    auto it = impl_->connections.find(connection_id);
    if (it != impl_->connections.end()) {
        return it->second->device_name;
    }
    return "";
}

void WebSocketServer::startPingPong(int interval_seconds) {
    if (!impl_->ping_running.load()) {
        impl_->ping_running.store(true);
        impl_->ping_thread = std::thread(&WebSocketServer::Impl::pingPongLoop, impl_.get(), this);
    }
}

void WebSocketServer::stopPingPong() {
    if (impl_->ping_running.load()) {
        impl_->ping_running.store(false);
        if (impl_->ping_thread.joinable()) {
            impl_->ping_thread.join();
        }
    }
}

void WebSocketServer::handleNewConnection(int client_socket) {
    std::string connection_id = generateConnectionId();
    
    if (performWebSocketHandshake(client_socket, connection_id)) {
        std::lock_guard<std::mutex> lock(impl_->connections_mutex);
        auto connection = std::make_unique<WebSocketConnection>();
        connection->socket_fd = client_socket;
        connection->connection_id = connection_id;
        connection->last_ping = std::chrono::steady_clock::now();
        impl_->connections[connection_id] = std::move(connection);
        
        std::cout << "New WebSocket connection: " << connection_id << std::endl;
    } else {
        close(client_socket);
    }
}

void WebSocketServer::handleMessage(const std::string& connection_id, const std::string& raw_message) {
    // Parse JSON message
    try {
        // Simple JSON parsing for basic messages
        WebSocketMessage message;
        
        // Find type field
        size_t type_pos = raw_message.find("\"type\":\"");
        if (type_pos != std::string::npos) {
            size_t start = type_pos + 8;
            size_t end = raw_message.find("\"", start);
            if (end != std::string::npos) {
                message.type = raw_message.substr(start, end - start);
            }
        }
        
        // Find event field
        size_t event_pos = raw_message.find("\"event\":\"");
        if (event_pos != std::string::npos) {
            size_t start = event_pos + 9;
            size_t end = raw_message.find("\"", start);
            if (end != std::string::npos) {
                message.event = raw_message.substr(start, end - start);
            }
        }
        
        // Find data field
        size_t data_pos = raw_message.find("\"data\":");
        if (data_pos != std::string::npos) {
            size_t start = data_pos + 7;
            // Find the end of the data field (simple implementation)
            size_t brace_count = 0;
            bool in_string = false;
            size_t end = start;
            
            for (size_t i = start; i < raw_message.length(); ++i) {
                char c = raw_message[i];
                if (c == '"' && (i == 0 || raw_message[i-1] != '\\')) {
                    in_string = !in_string;
                }
                if (!in_string) {
                    if (c == '{') brace_count++;
                    else if (c == '}') {
                        if (brace_count > 0) brace_count--;
                        else {
                            end = i;
                            break;
                        }
                    }
                    else if (c == ',' && brace_count == 0) {
                        end = i;
                        break;
                    }
                }
            }
            
            if (end > start) {
                message.data = raw_message.substr(start, end - start);
            }
        }
        
        message.source_connection = connection_id;
        
        // Call appropriate handler
        auto handler_it = impl_->message_handlers.find(message.event);
        if (handler_it != impl_->message_handlers.end()) {
            handler_it->second(connection_id, message);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing WebSocket message: " << e.what() << std::endl;
    }
}

void WebSocketServer::closeConnection(const std::string& connection_id) {
    std::lock_guard<std::mutex> lock(impl_->connections_mutex);
    auto it = impl_->connections.find(connection_id);
    if (it != impl_->connections.end()) {
        close(it->second->socket_fd);
        impl_->connections.erase(it);
        std::cout << "WebSocket connection closed: " << connection_id << std::endl;
    }
}

bool WebSocketServer::performWebSocketHandshake(int client_socket, const std::string& connection_id) {
    (void)connection_id; // Suppress unused parameter warning
    
    char buffer[4096];
    ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        return false;
    }
    
    buffer[bytes_read] = '\0';
    std::string request(buffer);
    
    // Find WebSocket key
    size_t key_pos = request.find("Sec-WebSocket-Key: ");
    if (key_pos == std::string::npos) {
        return false;
    }
    
    size_t key_start = key_pos + 19;
    size_t key_end = request.find("\r\n", key_start);
    if (key_end == std::string::npos) {
        return false;
    }
    
    std::string websocket_key = request.substr(key_start, key_end - key_start);
    std::string accept_key = generateWebSocketAccept(websocket_key);
    
    // Send handshake response
    std::string response = 
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: " + accept_key + "\r\n"
        "\r\n";
    
    send(client_socket, response.c_str(), response.length(), 0);
    return true;
}

std::string WebSocketServer::generateConnectionId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::string id;
    for (int i = 0; i < 32; ++i) {
        int random_value = dis(gen);
        if (random_value < 10) {
            id += char('0' + random_value);
        } else {
            id += char('a' + random_value - 10);
        }
    }
    return id;
}

std::string WebSocketServer::parseWebSocketFrame(const std::vector<uint8_t>& frame, bool& is_complete) {
    is_complete = false;
    if (frame.size() < 2) {
        return "";
    }
    
    bool fin = (frame[0] & 0x80) != 0;
    uint8_t opcode = frame[0] & 0x0F;
    bool masked = (frame[1] & 0x80) != 0;
    uint64_t payload_length = frame[1] & 0x7F;
    
    size_t header_size = 2;
    
    if (payload_length == 126) {
        if (frame.size() < 4) return "";
        payload_length = (frame[2] << 8) | frame[3];
        header_size = 4;
    } else if (payload_length == 127) {
        if (frame.size() < 10) return "";
        payload_length = 0;
        for (int i = 0; i < 8; ++i) {
            payload_length = (payload_length << 8) | frame[2 + i];
        }
        header_size = 10;
    }
    
    if (masked) {
        header_size += 4;
    }
    
    if (frame.size() < header_size + payload_length) {
        return ""; // Incomplete frame
    }
    
    std::string payload;
    if (masked && frame.size() >= header_size) {
        uint8_t mask[4] = {
            frame[header_size - 4],
            frame[header_size - 3], 
            frame[header_size - 2],
            frame[header_size - 1]
        };
        
        for (uint64_t i = 0; i < payload_length; ++i) {
            payload += char(frame[header_size + i] ^ mask[i % 4]);
        }
    } else {
        for (uint64_t i = 0; i < payload_length; ++i) {
            payload += char(frame[header_size + i]);
        }
    }
    
    is_complete = fin && (opcode == 1 || opcode == 2); // Text or binary frame
    return payload;
}

std::vector<uint8_t> WebSocketServer::createWebSocketFrame(const std::string& message) {
    std::vector<uint8_t> frame;
    
    // First byte: FIN (1) + RSV (000) + opcode (0001 for text)
    frame.push_back(0x81);
    
    // Payload length
    size_t length = message.length();
    if (length < 126) {
        frame.push_back(static_cast<uint8_t>(length));
    } else if (length < 65536) {
        frame.push_back(126);
        frame.push_back(static_cast<uint8_t>(length >> 8));
        frame.push_back(static_cast<uint8_t>(length & 0xFF));
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; --i) {
            frame.push_back(static_cast<uint8_t>((length >> (i * 8)) & 0xFF));
        }
    }
    
    // Payload data
    for (char c : message) {
        frame.push_back(static_cast<uint8_t>(c));
    }
    
    return frame;
}

void WebSocketServer::pingPongLoop() {
    while (impl_->ping_running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        
        auto now = std::chrono::steady_clock::now();
        std::vector<std::string> to_remove;
        
        {
            std::lock_guard<std::mutex> lock(impl_->connections_mutex);
            for (const auto& pair : impl_->connections) {
                auto time_since_ping = std::chrono::duration_cast<std::chrono::seconds>(
                    now - pair.second->last_ping).count();
                
                if (time_since_ping > 60) { // 60 seconds timeout
                    to_remove.push_back(pair.first);
                } else {
                    // Send ping
                    std::vector<uint8_t> ping_frame = {0x89, 0x00}; // Ping frame
                    send(pair.second->socket_fd, ping_frame.data(), ping_frame.size(), 0);
                }
            }
        }
        
        // Remove timed out connections
        for (const auto& conn_id : to_remove) {
            closeConnection(conn_id);
        }
    }
}

// Server loop implementation
void WebSocketServer::Impl::serverLoop(WebSocketServer* server) {
    while (running.load()) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_socket < 0) {
            if (running.load()) {
                std::cerr << "WebSocket accept failed" << std::endl;
            }
            continue;
        }
        
        // Handle connection in separate thread
        std::thread([this, server, client_socket]() {
            server->handleNewConnection(client_socket);
            
            // Message receiving loop
            char buffer[4096];
            std::vector<uint8_t> frame_buffer;
            
            while (true) {
                ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
                if (bytes_read <= 0) {
                    break;
                }
                
                // Add to frame buffer
                for (ssize_t i = 0; i < bytes_read; ++i) {
                    frame_buffer.push_back(static_cast<uint8_t>(buffer[i]));
                }
                
                // Try to parse complete frames
                bool is_complete = false;
                std::string message = server->parseWebSocketFrame(frame_buffer, is_complete);
                
                if (is_complete && !message.empty()) {
                    // Find connection ID for this socket
                    std::string connection_id;
                    {
                        std::lock_guard<std::mutex> lock(connections_mutex);
                        for (const auto& pair : connections) {
                            if (pair.second->socket_fd == client_socket) {
                                connection_id = pair.first;
                                break;
                            }
                        }
                    }
                    
                    if (!connection_id.empty()) {
                        server->handleMessage(connection_id, message);
                    }
                    
                    frame_buffer.clear();
                }
            }
            
            // Find and close connection
            std::string connection_id;
            {
                std::lock_guard<std::mutex> lock(connections_mutex);
                for (const auto& pair : connections) {
                    if (pair.second->socket_fd == client_socket) {
                        connection_id = pair.first;
                        break;
                    }
                }
            }
            
            if (!connection_id.empty()) {
                server->closeConnection(connection_id);
            }
        }).detach();
    }
}

void WebSocketServer::Impl::pingPongLoop(WebSocketServer* server) {
    server->pingPongLoop();
}

// Utility functions
std::string base64Encode(const std::string& data) {
    static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int val = 0, valb = -6;
    for (unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (result.size() % 4) {
        result.push_back('=');
    }
    return result;
}

std::string sha1Hash(const std::string& data) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), hash);
    
    std::string result;
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
        result += static_cast<char>(hash[i]);
    }
    return result;
}

std::string generateWebSocketAccept(const std::string& key) {
    std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string combined = key + magic;
    std::string hashed = sha1Hash(combined);
    return base64Encode(hashed);
}