#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

struct WebSocketConnection {
    int socket_fd;
    std::string connection_id;
    std::string user_id;
    std::string device_name;
    std::unordered_set<std::string> subscribed_events;
    std::chrono::steady_clock::time_point last_ping;
    bool authenticated = false;
};

struct WebSocketMessage {
    std::string type;
    std::string event;
    std::string data;
    std::string target_user;  // For targeted messages
    std::string source_connection;
};

using WebSocketHandler = std::function<void(const std::string& connection_id, const WebSocketMessage& message)>;

class WebSocketServer {
public:
    WebSocketServer();
    ~WebSocketServer();
    
    // Server lifecycle
    bool start(int port = 8081);
    void stop();
    bool isRunning() const;
    
    // Message handling
    void setMessageHandler(const std::string& event_type, WebSocketHandler handler);
    void broadcastMessage(const std::string& event, const std::string& data);
    void sendToConnection(const std::string& connection_id, const std::string& event, const std::string& data);
    void sendToUser(const std::string& user_id, const std::string& event, const std::string& data);
    
    // Connection management
    std::vector<std::string> getActiveConnections() const;
    std::vector<std::string> getConnectedUsers() const;
    bool isUserConnected(const std::string& user_id) const;
    void disconnectUser(const std::string& user_id);
    
    // Authentication
    void setAuthHandler(std::function<bool(const std::string& token, std::string& user_id)> auth_handler);
    bool authenticateConnection(const std::string& connection_id, const std::string& token);
    
    // Event subscription
    void subscribeToEvent(const std::string& connection_id, const std::string& event);
    void unsubscribeFromEvent(const std::string& connection_id, const std::string& event);
    
    // Device management
    void setDeviceName(const std::string& connection_id, const std::string& device_name);
    std::string getDeviceName(const std::string& connection_id) const;
    
    // Ping/Pong for connection health
    void startPingPong(int interval_seconds = 30);
    void stopPingPong();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    void handleNewConnection(int client_socket);
    void handleMessage(const std::string& connection_id, const std::string& raw_message);
    void closeConnection(const std::string& connection_id);
    bool performWebSocketHandshake(int client_socket, const std::string& connection_id);
    std::string generateConnectionId();
    std::string parseWebSocketFrame(const std::vector<uint8_t>& frame, bool& is_complete);
    std::vector<uint8_t> createWebSocketFrame(const std::string& message);
    void pingPongLoop();
};

// Utility functions for WebSocket protocol
std::string base64Encode(const std::string& data);
std::string sha1Hash(const std::string& data);
std::string generateWebSocketAccept(const std::string& key);

#endif // WEBSOCKET_SERVER_H