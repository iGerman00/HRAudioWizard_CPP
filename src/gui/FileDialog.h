#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <functional>

class FileDialog {
public:
    FileDialog();
    ~FileDialog();
    
    // Show the file dialog window
    void Show(bool* p_open, std::function<void(const std::string&)> onFileSelected);
    
    // Set the file extensions to filter
    void SetExtensionFilter(const std::vector<std::string>& extensions);
    
private:
    std::filesystem::path currentPath;
    std::vector<std::string> extensionFilter;
    std::string selectedFile;
    
    // Helper to check if a file has a valid extension
    bool HasValidExtension(const std::filesystem::path& path) const;
    
    // Navigate to a directory
    void NavigateTo(const std::filesystem::path& path);
};