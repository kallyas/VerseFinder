#include "HttpClient.h"
#include <curl/curl.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <future>

struct HttpClient::Impl {
    CURL* curl;
    long timeout;
    std::string user_agent;
    
    Impl() : curl(nullptr), timeout(30), user_agent("VerseFinder/2.0") {
        curl = curl_easy_init();
        if (!curl) {
            throw std::runtime_error("Failed to initialize libcurl");
        }
    }
    
    ~Impl() {
        if (curl) {
            curl_easy_cleanup(curl);
        }
    }
};

HttpClient::HttpClient() : pImpl(std::make_unique<Impl>()) {
}

HttpClient::~HttpClient() = default;

size_t HttpClient::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t total_size = size * nmemb;
    userp->append(static_cast<char*>(contents), total_size);
    return total_size;
}

size_t HttpClient::WriteFileCallback(void* contents, size_t size, size_t nmemb, FILE* userp) {
    return fwrite(contents, size, nmemb, userp);
}

int HttpClient::CurlProgressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    if (clientp && dltotal > 0) {
        auto* callback = static_cast<ProgressCallback*>(clientp);
        if (*callback) {
            double progress = dlnow / dltotal;
            (*callback)(progress);
        }
    }
    return 0;
}

std::string HttpClient::get(const std::string& url) {
    if (!pImpl->curl) {
        return "";
    }
    
    std::string response;
    
    curl_easy_setopt(pImpl->curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(pImpl->curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(pImpl->curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(pImpl->curl, CURLOPT_TIMEOUT, pImpl->timeout);
    curl_easy_setopt(pImpl->curl, CURLOPT_USERAGENT, pImpl->user_agent.c_str());
    curl_easy_setopt(pImpl->curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(pImpl->curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(pImpl->curl, CURLOPT_SSL_VERIFYHOST, 2L);
    
    CURLcode res = curl_easy_perform(pImpl->curl);
    
    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        return "";
    }
    
    long response_code;
    curl_easy_getinfo(pImpl->curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    if (response_code != 200) {
        std::cerr << "HTTP request failed with code: " << response_code << std::endl;
        return "";
    }
    
    return response;
}

void HttpClient::getAsync(const std::string& url, 
                         std::function<void(const std::string&)> onSuccess,
                         std::function<void(const std::string&)> onError,
                         ProgressCallback onProgress) {
    std::thread([this, url, onSuccess, onError, onProgress]() {
        try {
            std::string result = get(url);
            if (!result.empty() && onSuccess) {
                onSuccess(result);
            } else if (onError) {
                onError("Failed to fetch URL: " + url);
            }
        } catch (const std::exception& e) {
            if (onError) {
                onError(e.what());
            }
        }
    }).detach();
}

bool HttpClient::downloadFile(const std::string& url, const std::string& filepath,
                             ProgressCallback onProgress) {
    if (!pImpl->curl) {
        return false;
    }
    
    FILE* file = fopen(filepath.c_str(), "wb");
    if (!file) {
        return false;
    }
    
    curl_easy_setopt(pImpl->curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(pImpl->curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
    curl_easy_setopt(pImpl->curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(pImpl->curl, CURLOPT_TIMEOUT, pImpl->timeout);
    curl_easy_setopt(pImpl->curl, CURLOPT_USERAGENT, pImpl->user_agent.c_str());
    curl_easy_setopt(pImpl->curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(pImpl->curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(pImpl->curl, CURLOPT_SSL_VERIFYHOST, 2L);
    
    if (onProgress) {
        curl_easy_setopt(pImpl->curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(pImpl->curl, CURLOPT_PROGRESSFUNCTION, CurlProgressCallback);
        curl_easy_setopt(pImpl->curl, CURLOPT_PROGRESSDATA, &onProgress);
    }
    
    CURLcode res = curl_easy_perform(pImpl->curl);
    fclose(file);
    
    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }
    
    long response_code;
    curl_easy_getinfo(pImpl->curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    if (response_code != 200) {
        std::cerr << "HTTP download failed with code: " << response_code << std::endl;
        return false;
    }
    
    return true;
}

void HttpClient::setTimeout(long timeout_seconds) {
    pImpl->timeout = timeout_seconds;
}

void HttpClient::setUserAgent(const std::string& user_agent) {
    pImpl->user_agent = user_agent;
}