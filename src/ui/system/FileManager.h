#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <string>
#include <vector>

class FileManager {
public:
    FileManager();
    ~FileManager();

    // File operations
    static bool fileExists(const std::string& filepath);
    static bool createDirectory(const std::string& dirpath);
    static std::string readFile(const std::string& filepath);
    static bool writeFile(const std::string& filepath, const std::string& content);
    
    // Directory operations
    static std::vector<std::string> listFiles(const std::string& dirpath, const std::string& extension = "");
    static std::string getFileExtension(const std::string& filepath);
    static std::string getFileName(const std::string& filepath);
    static std::string getDirectoryPath(const std::string& filepath);
    
    // Utility methods
    static bool isValidFilePath(const std::string& filepath);
    static std::string sanitizeFileName(const std::string& filename);

private:
    static bool isValidCharacter(char c);
};

#endif // FILE_MANAGER_H