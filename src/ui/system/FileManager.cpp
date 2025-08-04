#include "FileManager.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>

FileManager::FileManager() {}
FileManager::~FileManager() {}

bool FileManager::fileExists(const std::string& filepath) {
    return std::filesystem::exists(filepath);
}

bool FileManager::createDirectory(const std::string& dirpath) {
    try {
        return std::filesystem::create_directories(dirpath);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create directory: " << dirpath << " - " << e.what() << std::endl;
        return false;
    }
}

std::string FileManager::readFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for reading: " << filepath << std::endl;
        return "";
    }
    
    std::string content;
    std::string line;
    while (std::getline(file, line)) {
        content += line + "\n";
    }
    
    return content;
}

bool FileManager::writeFile(const std::string& filepath, const std::string& content) {
    // Create directory if it doesn't exist
    std::string dirpath = getDirectoryPath(filepath);
    if (!dirpath.empty() && !fileExists(dirpath)) {
        createDirectory(dirpath);
    }
    
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filepath << std::endl;
        return false;
    }
    
    file << content;
    return file.good();
}

std::vector<std::string> FileManager::listFiles(const std::string& dirpath, const std::string& extension) {
    std::vector<std::string> files;
    
    try {
        if (!std::filesystem::exists(dirpath) || !std::filesystem::is_directory(dirpath)) {
            return files;
        }
        
        for (const auto& entry : std::filesystem::directory_iterator(dirpath)) {
            if (entry.is_regular_file()) {
                std::string filepath = entry.path().string();
                if (extension.empty() || getFileExtension(filepath) == extension) {
                    files.push_back(filepath);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to list files in directory: " << dirpath << " - " << e.what() << std::endl;
    }
    
    return files;
}

std::string FileManager::getFileExtension(const std::string& filepath) {
    std::filesystem::path path(filepath);
    std::string ext = path.extension().string();
    if (!ext.empty() && ext[0] == '.') {
        ext = ext.substr(1); // Remove the dot
    }
    return ext;
}

std::string FileManager::getFileName(const std::string& filepath) {
    std::filesystem::path path(filepath);
    return path.filename().string();
}

std::string FileManager::getDirectoryPath(const std::string& filepath) {
    std::filesystem::path path(filepath);
    return path.parent_path().string();
}

bool FileManager::isValidFilePath(const std::string& filepath) {
    if (filepath.empty()) {
        return false;
    }
    
    // Check for invalid characters
    for (char c : filepath) {
        if (!isValidCharacter(c)) {
            return false;
        }
    }
    
    // Additional checks for common invalid patterns
    if (filepath.find("..") != std::string::npos) {
        return false; // Prevent directory traversal
    }
    
    return true;
}

std::string FileManager::sanitizeFileName(const std::string& filename) {
    std::string sanitized;
    for (char c : filename) {
        if (isValidCharacter(c)) {
            sanitized += c;
        } else {
            sanitized += '_'; // Replace invalid characters with underscore
        }
    }
    return sanitized;
}

bool FileManager::isValidCharacter(char c) {
    // Allow alphanumeric characters, common punctuation, and path separators
    return std::isalnum(c) || c == '.' || c == '-' || c == '_' || 
           c == '/' || c == '\\' || c == ':' || c == ' ';
}