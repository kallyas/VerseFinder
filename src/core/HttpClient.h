#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <string>
#include <functional>
#include <memory>

class HttpClient {
public:
    using ProgressCallback = std::function<void(double progress)>;
    
    HttpClient();
    ~HttpClient();
    
    // Synchronous GET request
    std::string get(const std::string& url);
    
    // Asynchronous GET request with progress callback
    void getAsync(const std::string& url, 
                  std::function<void(const std::string&)> onSuccess,
                  std::function<void(const std::string&)> onError = nullptr,
                  ProgressCallback onProgress = nullptr);
    
    // Download file with progress
    bool downloadFile(const std::string& url, const std::string& filepath,
                      ProgressCallback onProgress = nullptr);
    
    // Set timeout in seconds
    void setTimeout(long timeout_seconds);
    
    // Set user agent
    void setUserAgent(const std::string& user_agent);
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
    
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    static size_t WriteFileCallback(void* contents, size_t size, size_t nmemb, FILE* userp);
    static int CurlProgressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);
};

#endif // HTTP_CLIENT_H